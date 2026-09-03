BLE ADV Monitor dla Windows 10
==============================

1. Zainstaluj Python 3.10+ (jeżeli nie masz).
2. Otwórz PowerShell.
3. Zainstaluj Bleak:

   py -m pip install bleak

   Jeżeli polecenie 'py' nie działa:
   python -m pip install bleak

4. Uruchom:

   py ble_adv_monitor.py

   albo:
   python ble_adv_monitor.py

5. Przerwanie skanowania: Ctrl+C

Program szuka Manufacturer Data z company ID 0xFFFF.
Bleak usuwa company ID z danych payloadu, więc dla Twojego firmware oczekiwane jest 14 bajtów:

  0..1   temperatura SHT41 #1, int16 LE, /100
  2..3   wilgotność SHT41 #1, int16 LE, /100
  4..5   temperatura SHT41 #2, int16 LE, /100
  6..7   wilgotność SHT41 #2, int16 LE, /100
  8..11  timestamp / uptime, uint32 LE, sekundy
 12..13  bateria, int16 LE, mV

Przykład:

14:20:01.123  #     1  RSSI  -51 dBm  delta ---       MFG 2C 09 34 16 ...
14:20:02.131  #     2  RSSI  -52 dBm  delta 1008.0 ms MFG 2C 09 34 16 ...

Jeżeli XIAO nadaje co 1 sekundę, odstępy powinny być w przybliżeniu ~1000 ms.
Nie oczekuj idealnie 1000 ms, ponieważ BLE stosuje niewielki losowy offset między zdarzeniami advertising.

Uwaga:
Windows/sterownik Bluetooth może filtrować lub agregować część reklam. Jeżeli wyniki będą nadal bardzo rzadkie, warto porównać ten skaner z innym adapterem BLE.
