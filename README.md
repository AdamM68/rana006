# 🌡️ Microclimate Under Wound Dressing: 24h BLE Monitoring System

Projekt badawczo-rozwojowy dotyczący ciągłego pomiaru i analizy mikroklimatu (temperatury i wilgotności względnej) pod opatrunkiem rany. System wykorzystuje ultraprecyzyjne czujniki oraz mikrokontroler z łącznością Bluetooth Low Energy (BLE) do ciągłej rejestracji parametrów w czasie rzeczywistym.

---

## 📌 O projekcie

Temperatura i wilgotność w otoczeniu rany mają kluczowe znaczenie dla dynamiki procesu gojenia oraz ryzyka rozwoju infekcji bakteryjnych. Tradycyjne opatrunki stwarzają specyficzny mikroklimat, jednak jego dokładna charakterystyka wymaga ciągłego i nieinwazyjnego pomiaru.

Projekt ten służy do weryfikacji hipotezy, **czy stosowanie opatrunku wprowadza istotną statystycznie zmianę temperatury i wilgotności w okolicy rany** w porównaniu do odsłoniętej skóry w takich samych warunkach otoczenia.

---

## 🎯 Cel naukowy i hipoteza

1. **Cel główny:** Ocena wpływu obecności opatrunku na parametry mikroklimatu skóry w ujęciu dobowym (24h).
2. **Hipoteza badawcza:** Opatrunek rany powoduje istotny statystycznie (statistically significant) wzrost wilgotności i/lub temperatury w stosunku do punktu kontrolnego na nieosłoniętej skórze.
3. **Zastosowanie praktyczne:** Wyniki pomogą ocenić właściwości paroprzepuszczalne i termiczne badanych opatrunków, co może posłużyć do projektowania nowoczesnych „inteligentnych opatrunków” (smart dressings).

---

## 🛠️ Architektura sprzętowa i specyfikacja

* **Mikrokontroler:** `nRF52840` (Nordic Semiconductor)
  * Wysoka wydajność energetyczna i wbudowane wsparcie dla BLE 5.0+.
  * Niewielki rozmiar umożliwiający komfortowy montaż na ciele badanego.
* **Czujniki środowiskowe:** 2x `Sensirion SHT41`
  * **Dokładność wilgotności:** ±1.5% RH
  * **Dokładność temperatury:** ±0.1°C
  * **Interfejs komunikacyjny:** I2C
* **Sposób rozmieszczenia czujników:**
  1. **Punkt A (Eksperymentalny):** Czujnik SHT41 umieszczony pod opatrunkiem rany.
  2. **Punkt B (Kontrolny):** Czujnik SHT41 umieszczony na odsłoniętej skórze w bezpośrednim sąsiedztwie opatrunku.

---

## 📡 Metodologia i ciągłość pomiaru

* **Czas trwania eksperymentu:** 24 godziny (ciągły zapis dobowy).
* **Częstotliwość próbkowania:** Pomiar dokonywany i wysyłany co **30 sekund** (łącznie 2880 punktów pomiarowych na każdy czujnik w ciągu doby).
* **Komunikacja bezprzewodowa:** Pakiet danych transmitowany przez **BLE (Bluetooth Low Energy)** do dedykowanego odbiornika / aplikacji rejestrującej (Gateway / Smartphone).
* **Format pakietu BLE:** Zawiera znacznik czasu (timestamp), identyfikator czujnika oraz surowe/przeliczone wartości $T$ i $RH$.

---

## 📊 Analiza danych

Zbierane dane pozwalają na:
* Wyznaczenie różnic średnich, median oraz odchyleń standardowych ($T$ i $RH$) pomiędzy punktem testowym a kontrolnym.
* Przeprowadzenie testów hipotez statystycznych (np. test *t*-Studenta dla prób zależnych lub test Wilcoxona) w celu określenia istotności różnic.
* Wizualizację profilu dobowego (wykresy liniowe zmian temperatury i wilgotności w czasie).

---

## 🚀 Jak uruchomić projekt?

1. **Wymagania sprzętowe:**
   * Płytka z nRF52840 (np. nRF52840 DK, Seeed Studio XIAO nRF52840).
   * 2x moduł SHT41 z magistralą I2C (lub własna płytka PCB).
2. **Oprogramowanie:**
   * Zephyr RTOS / nRF Connect SDK / Arduino IDE z rdzeniem nRF52.
3. **Kroki:**
   * Wgraj firmware umieszczony w katalogu `/firmware`. // do zrobienia
   * Uruchom aplikację nRF Logger 
