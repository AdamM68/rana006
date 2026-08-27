#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>

// Deklaracja czujników temperatury
const struct device *sht41_1_dev = DEVICE_DT_GET(DT_NODELABEL(sht41_1));
const struct device *sht41_2_dev = DEVICE_DT_GET(DT_NODELABEL(sht41_2));

// Specyfikacja kanału ADC oraz pinu sterującego z Devicetree (rana004)
static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
static const struct gpio_dt_spec vbat_en = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), vbat_enable_gpios);

// Timery opóźnione
struct k_work_delayable sensor_work;    // Pomiary co 60 sekund
struct k_work_delayable batt_work;      // Odczyt napięcia co 30 sekund
struct k_work_delayable adv_stop_work;  // Wyłączenie radia po 30 sekundach

struct bt_conn *current_conn = NULL;

// UUID serwisu i charakterystyk
#define BT_UUID_CUSTOM_SERVICE_VAL BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)
#define BT_UUID_CUSTOM_CHAR_VAL    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1) // Czujniki (8 bajtów)
#define BT_UUID_CUSTOM_BATT_VAL    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2) // Bateria (2 bajty)

static struct bt_uuid_128 custom_svc_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_SERVICE_VAL);
static struct bt_uuid_128 custom_char_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_CHAR_VAL);
static struct bt_uuid_128 custom_batt_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_BATT_VAL);

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {}

// Definicja serwisu BLE GATT z dwiema charakterystykami NOTIFY
BT_GATT_SERVICE_DEFINE(custom_sensor_svc,
    BT_GATT_PRIMARY_SERVICE(&custom_svc_uuid),
    
    // Charakterystyka nr 1 (Czujniki SHT41) -> attrs[1]
    BT_GATT_CHARACTERISTIC(&custom_char_uuid.uuid, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    
    // Charakterystyka nr 2 (Napięcie Baterii) -> attrs[4]
    BT_GATT_CHARACTERISTIC(&custom_batt_uuid.uuid, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
};
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void adv_stop_handler(struct k_work *work) {
    bt_le_adv_stop(); 
}

static void connected(struct bt_conn *conn, uint8_t err) {
    if (err) return;
    current_conn = bt_conn_ref(conn);
    k_work_cancel_delayable(&adv_stop_work);

    struct bt_le_conn_param param = {
        .interval_min = 80, .interval_max = 160, .latency = 49, .timeout = 600
    };
    bt_conn_le_param_update(conn, &param);
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
    bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    k_work_reschedule(&adv_stop_work, K_SECONDS(30));
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

// ==============================================================================
// 1. LOGIKA BATERII Z RANA004 - odczyt co 120s
// ==============================================================================
int init_battery_measuring(void) {
    int err;
    if (!adc_is_ready_dt(&adc_channel)) return -ENODEV;
    err = adc_channel_setup_dt(&adc_channel);
    if (err < 0) return err;

    if (!gpio_is_ready_dt(&vbat_en)) return -ENODEV;
    err = gpio_pin_configure_dt(&vbat_en, GPIO_OUTPUT_ACTIVE);
    if (err < 0) return err;

    k_sleep(K_MSEC(1));
    return 0;
}

int read_battery_voltage(void) {
    int err;
    uint16_t buf;
    struct adc_sequence sequence = {
        .options = NULL,
        .channels = BIT(adc_channel.channel_id),
        .buffer = &buf,
        .buffer_size = sizeof(buf),
        .resolution = 12,
    };

    err = adc_read(adc_channel.dev, &sequence);
    if (err) return err;

    int32_t raw_val = buf;
    int32_t pin_mv = (raw_val * 3600) / 4095;
    int32_t bat_mv = pin_mv * 1510 / 510; 
    
    return bat_mv;
}

// Timer obsługujący wysyłkę baterii (co xx sekund)
static void batt_work_handler(struct k_work *work) {
    if (current_conn) {
        // Włączamy dzielnik na czas pomiaru
        gpio_pin_set_dt(&vbat_en, 1);
        k_sleep(K_MSEC(2)); 

        int32_t vbat_mv = read_battery_voltage();
        
        // Wyłączamy dzielnik po pomiarze
        gpio_pin_set_dt(&vbat_en, 0);

        if (vbat_mv >= 0) {
            uint8_t payload[2];
            int16_t out_mv = (int16_t)vbat_mv;
            payload[0] = out_mv & 0xFF;
            payload[1] = (out_mv >> 8) & 0xFF;
            
            // Wysłanie charakterystyki nr 2 (attrs[4])
            bt_gatt_notify(current_conn, &custom_sensor_svc.attrs[4], payload, sizeof(payload));
        }
    }
    k_work_reschedule(&batt_work, K_SECONDS(120)); // kolejny odczyt za 120s
}

// ==============================================================================
// 2. PĘTLA ODCZYTU CZUJNIKÓW SHT41 (odczyt co 30 sekund)
// ==============================================================================
static void sensor_work_handler(struct k_work *work) {
    if (current_conn) {
        struct sensor_value temp, hum;
        uint8_t payload[12];
        
        // CZUJNIK 1
        if (sht41_1_dev && sensor_sample_fetch(sht41_1_dev) == 0) {
            sensor_channel_get(sht41_1_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            sensor_channel_get(sht41_1_dev, SENSOR_CHAN_HUMIDITY, &hum);
            int16_t t_out = (int16_t)(sensor_value_to_double(&temp) * 100.0);
            int16_t h_out = (int16_t)(sensor_value_to_double(&hum) * 100.0);
            payload[0] = t_out & 0xFF; payload[1] = (t_out >> 8) & 0xFF;
            payload[2] = h_out & 0xFF; payload[3] = (h_out >> 8) & 0xFF;
        } else {
            int16_t err_val = -9999;
            payload[0] = err_val & 0xFF; payload[1] = (err_val >> 8) & 0xFF;
            payload[2] = err_val & 0xFF; payload[3] = (err_val >> 8) & 0xFF;
        }

        // CZUJNIK 2
        if (sht41_2_dev && sensor_sample_fetch(sht41_2_dev) == 0) {
            sensor_channel_get(sht41_2_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            sensor_channel_get(sht41_2_dev, SENSOR_CHAN_HUMIDITY, &hum);
            int16_t t_out = (int16_t)(sensor_value_to_double(&temp) * 100.0);
            int16_t h_out = (int16_t)(sensor_value_to_double(&hum) * 100.0);
            payload[4] = t_out & 0xFF; payload[5] = (t_out >> 8) & 0xFF;
            payload[6] = h_out & 0xFF; payload[7] = (h_out >> 8) & 0xFF;
        } else {
            int16_t err_val = -9999;
            payload[4] = err_val & 0xFF; payload[5] = (err_val >> 8) & 0xFF;
            payload[6] = err_val & 0xFF; payload[7] = (err_val >> 8) & 0xFF;
        }

        // --- DODANO: Znak czasu (Timestamp w SEKUNDACH) ---
        // Pobieramy czas od uruchomienia urządzenia i konwertujemy na sekundy
        uint32_t uptime_sec = k_uptime_get_32() / 1000;
        
        // Pakowanie 32-bitowej liczby (Little Endian)
        payload[8] = uptime_sec & 0xFF;
        payload[9] = (uptime_sec >> 8) & 0xFF;
        payload[10] = (uptime_sec >> 16) & 0xFF;
        payload[11] = (uptime_sec >> 24) & 0xFF;
        
        // Wysłanie charakterystyki nr 1 (attrs[1])
        bt_gatt_notify(current_conn, &custom_sensor_svc.attrs[1], payload, sizeof(payload));
    }
    k_work_reschedule(&sensor_work, K_SECONDS(30)); // kolejny odczyt za 30 sekund
}
// ==============================================================================
// 3. MAIN
// ==============================================================================

int main(void) {
    // 1. Inicjalizacja Twojego sprzętu ADC z rana004
    init_battery_measuring();
    // Upewniamy się, że dzielnik jest wyłączony, gdy nie mierzymy
    gpio_pin_set_dt(&vbat_en, 0); 

    // 2. Inicjalizacja BLE i timerów
    bt_enable(NULL);
    k_work_init_delayable(&adv_stop_work, adv_stop_handler);
    k_work_init_delayable(&sensor_work, sensor_work_handler);
    k_work_init_delayable(&batt_work, batt_work_handler);

    // 3. Start systemu
    bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    
    k_work_reschedule(&adv_stop_work, K_SECONDS(30));
    k_work_reschedule(&sensor_work, K_SECONDS(10)); // pierwszy odczyt sht po 10s
    k_work_reschedule(&batt_work, K_SECONDS(20));   // pierwszy odczyt batt po 20s

    return 0;
}