import asyncio
import sys
from datetime import datetime
from bleak import BleakScanner

COMPANY_ID = 0xFFFF


def int16_le(data, i):
    v = data[i] | (data[i + 1] << 8)
    return v - 0x10000 if v & 0x8000 else v


def uint32_le(data, i):
    return int.from_bytes(data[i:i + 4], byteorder="little", signed=False)


def hex_bytes(data):
    return " ".join(f"{b:02X}" for b in data)


last_packet_monotonic = None
packet_count = 0


def handle_detection(device, advertisement_data):
    global last_packet_monotonic, packet_count

    # Bleak usually exposes Manufacturer Data as:
    # {company_id: payload_without_company_id}
    mfg = advertisement_data.manufacturer_data
    if COMPANY_ID not in mfg:
        return

    data = bytes(mfg[COMPANY_ID])
    now_mono = asyncio.get_running_loop().time()
    wall = datetime.now().strftime("%H:%M:%S.%f")[:-3]

    if last_packet_monotonic is None:
        delta_text = "---"
    else:
        delta_ms = (now_mono - last_packet_monotonic) * 1000.0
        delta_text = f"{delta_ms:8.1f} ms"
    last_packet_monotonic = now_mono
    packet_count += 1

    print(f"{wall}  #{packet_count:6d}  RSSI {advertisement_data.rssi:4d} dBm  "
          f"delta {delta_text}  MFG {hex_bytes(data)}")

    # Expected payload after company ID:
    #  0..1   SHT41 #1 temperature, x100, int16 LE
    #  2..3   SHT41 #1 humidity, x100, int16 LE
    #  4..5   SHT41 #2 temperature, x100, int16 LE
    #  6..7   SHT41 #2 humidity, x100, int16 LE
    #  8..11  uptime seconds, uint32 LE
    # 12..13  battery mV, int16 LE
    if len(data) >= 14:
        t1 = int16_le(data, 0) / 100.0
        h1 = int16_le(data, 2) / 100.0
        t2 = int16_le(data, 4) / 100.0
        h2 = int16_le(data, 6) / 100.0
        uptime = uint32_le(data, 8)
        battery = int16_le(data, 12)

        print(
            f"             T1={t1:7.2f} C  H1={h1:7.2f} %  "
            f"T2={t2:7.2f} C  H2={h2:7.2f} %  "
            f"uptime={uptime:8d} s  battery={battery:5d} mV"
        )


async def main():
    print("BLE ADV monitor - Windows / Bleak")
    print(f"Szukam Manufacturer Data company ID 0x{COMPANY_ID:04X}")
    print("Przerwij Ctrl+C\n")
    print("TIME           PACKET    RSSI       INTERVAL          MANUFACTURER DATA")
    print("-" * 110)

    try:
        scanner = BleakScanner(detection_callback=handle_detection)
        await scanner.start()
    except Exception as e:
        print("Nie udało się uruchomić skanera BLE:")
        print(repr(e))
        print("\nSprawdź, czy Windows ma działający adapter Bluetooth i czy pakiet 'bleak' jest zainstalowany.")
        sys.exit(1)

    try:
        while True:
            await asyncio.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        try:
            await scanner.stop()
        except Exception:
            pass
        print("\nSkanowanie zatrzymane.")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nSkanowanie zatrzymane.")
