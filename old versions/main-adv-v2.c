// wersja ble ADV - bardziej energo oszczędna, nie potrzebuje podtrzymania połączenia GATT, wysyła 1 pakiet co 30s
// ver 2 - korekta sposobu nadawania pakietu ble adv
// nRF SDK 3.4.0

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>

// Deklaracja czujnika z drzewa urządzeń (Device Tree)
const struct device *sht41_1_dev = DEVICE_DT_GET(DT_NODELABEL(sht41_1));
const struct device *sht41_2_dev = DEVICE_DT_GET(DT_NODELABEL(sht41_2));

// Specyfikacja kanału ADC oraz pinu sterującego z Devicetree (rana004)
static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
static const struct gpio_dt_spec vbat_en = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), vbat_enable_gpios);

// Timery (Workqueue) do zadań opóźnionych
struct k_work_delayable sensor_work;    // Pomiary i wysyłka co 30 sekund

// Zmienna przechowująca dane rozgłoszeniowe (2 bajty ID + 14 bajtów payloadu)
static uint8_t mfg_data[16] = {
    0xFF, 0xFF, // ID Producenta (0xFFFF - domyślne testowe)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Pakiety rozgłoszeniowe (Advertising) - flaga i dane producenta z pomiarami
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
};

// Pakiety odpowiedzi (Scan Response) - nazwa urządzenia widoczna w skanerze
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

// ==============================================================================
// 1. LOGIKA BATERII Z RANA004
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

// ==============================================================================
// 2. PĘTLA ODCZYTU (Wspólny pakiet rozgłoszeniowy co 30 sekund)
// ==============================================================================
static void sensor_work_handler(struct k_work *work) {
    struct sensor_value temp, hum;
    
    // --- Odczyt Czujnika 1 (i2c0) ---
    if (sht41_1_dev && sensor_sample_fetch(sht41_1_dev) == 0) {
        sensor_channel_get(sht41_1_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        sensor_channel_get(sht41_1_dev, SENSOR_CHAN_HUMIDITY, &hum);
        int16_t t_out = (int16_t)(sensor_value_to_double(&temp) * 100.0);
        int16_t h_out = (int16_t)(sensor_value_to_double(&hum) * 100.0);
        mfg_data[2] = t_out & 0xFF; mfg_data[3] = (t_out >> 8) & 0xFF;
        mfg_data[4] = h_out & 0xFF; mfg_data[5] = (h_out >> 8) & 0xFF;
    } else {
        int16_t err_val = -9999;
        mfg_data[2] = err_val & 0xFF; mfg_data[3] = (err_val >> 8) & 0xFF;
        mfg_data[4] = err_val & 0xFF; mfg_data[5] = (err_val >> 8) & 0xFF;
    }

    // --- Odczyt Czujnika 2 (i2c1) ---
    if (sht41_2_dev && sensor_sample_fetch(sht41_2_dev) == 0) {
        sensor_channel_get(sht41_2_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        sensor_channel_get(sht41_2_dev, SENSOR_CHAN_HUMIDITY, &hum);
        int16_t t_out = (int16_t)(sensor_value_to_double(&temp) * 100.0);
        int16_t h_out = (int16_t)(sensor_value_to_double(&hum) * 100.0);
        mfg_data[6] = t_out & 0xFF; mfg_data[7] = (t_out >> 8) & 0xFF;
        mfg_data[8] = h_out & 0xFF; mfg_data[9] = (h_out >> 8) & 0xFF;
    } else {
        int16_t err_val = -9999;
        mfg_data[6] = err_val & 0xFF; mfg_data[7] = (err_val >> 8) & 0xFF;
        mfg_data[8] = err_val & 0xFF; mfg_data[9] = (err_val >> 8) & 0xFF;
    }

    // --- Znak czasu (Timestamp w SEKUNDACH) ---
    uint32_t uptime_sec = k_uptime_get_32() / 1000;
    
    mfg_data[10] = uptime_sec & 0xFF;
    mfg_data[11] = (uptime_sec >> 8) & 0xFF;
    mfg_data[12] = (uptime_sec >> 16) & 0xFF;
    mfg_data[13] = (uptime_sec >> 24) & 0xFF;

    // --- Odczyt Baterii ---
    gpio_pin_set_dt(&vbat_en, 1);
    k_sleep(K_MSEC(2));
    int32_t vbat_mv = read_battery_voltage();
    gpio_pin_set_dt(&vbat_en, 0);

    int16_t out_mv = (vbat_mv >= 0) ? (int16_t)vbat_mv : -9999;
    mfg_data[14] = out_mv & 0xFF;
    mfg_data[15] = (out_mv >> 8) & 0xFF;

    // Zastępujemy makro NCONN własną, poprawną strukturą (Scannable)
    // --- Emisja pakietu (Beacon ze Scan Response) ---
    //struct bt_le_adv_param adv_param = {
    //    .id = BT_ID_DEFAULT,
    //    .sid = 0,
    //    .secondary_max_skip = 0,
        // Opcja SCANNABLE pozwala na użycie tablicy 'sd'
        // Opcja USE_IDENTITY wymusza stały adres MAC (Twój 68:CE)
    //    .options = BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_USE_IDENTITY,
    //    .interval_min = 0x00A0, /* 100 ms */
    //    .interval_max = 0x00F0, /* 150 ms */
    //    .peer = NULL,
    //};

    // bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    //bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    
    // Czas na rozgłoszenie pakietu w eter
    //k_sleep(K_MSEC(500));
    
    // Usypiamy radio BLE całkowicie na resztę czasu
    //bt_le_adv_stop();

    // bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

    // Kolejny odczyt za 30 sekund
    k_work_reschedule(&sensor_work, K_SECONDS(5));
}

static void bt_ready(int err)
{
    if (err) {
        return;
    }

    struct bt_le_adv_param adv_param = {
        .id = BT_ID_DEFAULT,
        .sid = 0,
        .secondary_max_skip = 0,
        .options = BT_LE_ADV_OPT_SCANNABLE |
                   BT_LE_ADV_OPT_USE_IDENTITY,
        .interval_min = 0x00A0,
        .interval_max = 0x00F0,
        .peer = NULL,
    };

    bt_le_adv_start(&adv_param,
                    ad, ARRAY_SIZE(ad),
                    sd, ARRAY_SIZE(sd));

}

// ==============================================================================
// 3. MAIN
// ==============================================================================
int main(void) {
    init_battery_measuring();
    gpio_pin_set_dt(&vbat_en, 0);

    k_work_init_delayable(&sensor_work, sensor_work_handler);
    
    bt_enable(bt_ready);
    
    k_work_reschedule(&sensor_work, K_SECONDS(1)); 

    return 0;
}