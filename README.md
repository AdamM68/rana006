# 🌡️ Microclimate Under Wound Dressing: 24h BLE Monitoring System Experiment

# 🌡️ Mikroklimat pod opatrunkiem rany: 24-godzinny eksperyment z systemem monitorowania BLE

---
![Temp&Humid BLE monitor](images/bluth.jpg)

## 🇵🇱 Wersja Polska

Projekt badawczo-rozwojowy dotyczący ciągłego pomiaru i analizy mikroklimatu (temperatury i wilgotności względnej) pod opatrunkiem rany. System wykorzystuje ultraprecyzyjne czujniki oraz mikrokontroler z łącznością Bluetooth Low Energy (BLE) do ciągłej rejestracji parametrów w czasie rzeczywistym.

UWAGA: 11.08.2026
-- projekt jest wciąż na etapie budowy, 
-- planuje dodanie timestamp dla łatwiejszej analizy pomiarów ... ok, zrobione 
-- w planie udostępnienie skompilowanych plików .uf2 dla płytki xiao,
-- w następnej kolejności publikacja uzyskanych pomiarów i analiza statystyczna wyników
-- dodanie zdjęć eksperymentu i detali technicznych 

Zapraszam do kontaktu i wspólnego rozwijania projektu/eksperymentu

dr n.med. Adam Mościcki  adam68.moscicki@gmail.com 

---

## 📌 O projekcie

Temperatura i wilgotność w otoczeniu rany mają kluczowe znaczenie dla dynamiki procesu gojenia oraz ryzyka rozwoju infekcji bakteryjnych. Tradycyjne opatrunki stwarzają specyficzny mikroklimat, jednak jego dokładna charakterystyka wymaga ciągłego i nieinwazyjnego pomiaru.

Projekt ten służy do weryfikacji hipotezy, **czy stosowanie opatrunku wprowadza istotną statystycznie zmianę temperatury i wilgotności w okolicy rany** w porównaniu do odsłoniętej skóry w takich samych warunkach otoczenia.

---

## 🎯 Cel naukowy i hipoteza

1. **Cel główny:** Ocena wpływu obecności opatrunku na parametry mikroklimatu skóry w ujęciu dobowym (24h).
2. **Hipoteza badawcza:** Opatrunek rany powoduje istotny statystycznie wzrost wilgotności i/lub temperatury w stosunku do punktu kontrolnego na nieosłoniętej skórze.
3. **Zastosowanie praktyczne:** Wyniki pomogą ocenić właściwości paroprzepuszczalne i termiczne badanych opatrunków, co może posłużyć do projektowania nowoczesnych „inteligentnych opatrunków” (smart dressings).

---

## 🛠️ Architektura sprzętowa i specyfikacja

* **Mikrokontroler:** `nRF52840` (Nordic Semiconductor) *(Xiao nRF52840)*
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
   * Uruchom aplikację nRF Logger lub dedykowany loger: `ble-sht41.html` 
   * Uwaga! Android może blokować długie utrzymanie połączenia Web-BLE przez stronę html. Osobiście udało mi sie utrzymać aktywne połączenie - logowanie przez planowany czas 24h. Być może zawdzięczam to podłączeniu smartfona do zasilania - w takiej konfiguracji Android może utrzymać połączenie BLE dłużej. W razie kłopotów z utrzymaniem dłuższego połączenia - logowania poprzez stronę ble.sht41.html należy użyć aplikacji nRF Connect + nRF Logger a następnie przygotować plik logu do dalszej analizy poprzez skrypt python, który umieściłem w katalogu serwisowym.
   * Analiza danych ...

## Wyniki

...

---
---

## 🇬🇧 English Version

## 📌 About the Project

Research and development project focused on the continuous measurement and analysis of the microclimate (temperature and relative humidity) under a wound dressing. The system utilizes ultra-precise sensors and a microcontroller with Bluetooth Low Energy (BLE) connectivity for real-time continuous parameter logging.

NOTE: August 11, 2026
-- The project is still under construction,
-- Planning to add a timestamp for easier measurement analysis ... ok, done
-- Planned to release compiled .uf2 files for the XIAO board,
-- Next steps include publishing the obtained measurements and statistical analysis of the results
-- Adding experiment photos and technical details

Feel free to contact me and collaborate on developing the project/experiment.

Adam Mościcki, MD, PhD  adam68.moscicki@gmail.com 

---

## 🎯 Scientific Objective and Hypothesis

1. **Main Objective:** Evaluation of the impact of a dressing on skin microclimate parameters over a 24-hour daily cycle.
2. **Research Hypothesis:** The wound dressing causes a statistically significant increase in humidity and/or temperature relative to a control point on uncovered skin.
3. **Practical Application:** The results will help assess the water vapor permeability and thermal properties of the tested dressings, which can be used to design modern "smart dressings".

---

## 🛠️ Hardware Architecture and Specification

* **Microcontroller:** `nRF52840` (Nordic Semiconductor) *(XIAO nRF52840)*
  * High energy efficiency and built-in support for BLE 5.0+.
  * Small size enabling comfortable mounting on the subject's body.
* **Environmental Sensors:** 2x `Sensirion SHT41`
  * **Humidity Accuracy:** ±1.5% RH
  * **Temperature Accuracy:** ±0.1°C
  * **Communication Interface:** I2C
* **Sensor Placement:**
  * **Point A (Experimental):** SHT41 sensor placed under the wound dressing.
  * **Point B (Control):** SHT41 sensor placed on exposed skin in the immediate vicinity of the dressing.

---

## 📡 Methodology and Measurement Continuity

* **Experiment Duration:** 24 hours (continuous daily logging).
* **Sampling Frequency:** Measurements taken and transmitted every **30 seconds** (total of 2880 measurement points per sensor over a 24-hour period).
* **Wireless Communication:** Data packet transmitted via **BLE (Bluetooth Low Energy)** to a dedicated receiver / logging application (Gateway / Smartphone).
* **BLE Packet Format:** Contains a timestamp, sensor ID, and raw/converted $T$ and $RH$ values.

---

## 📊 Data Analysis

The collected data allows for:
* Determining differences in means, medians, and standard deviations ($T$ and $RH$) between the test and control points.
* Performing statistical hypothesis tests (e.g., Student's *t*-test for paired samples or Wilcoxon test) to determine the significance of differences.
* Visualizing the daily profile (line charts of temperature and humidity changes over time).

---

## 🚀 How to Run the Project?

1. **Hardware Requirements:**
   * nRF52840 board (e.g., nRF52840 DK, Seeed Studio XIAO nRF52840).
   * 2x SHT41 I2C modules (or custom PCB).
2. **Software:**
   * Zephyr RTOS / nRF Connect SDK / Arduino IDE with nRF52 core.
3. **Steps:**
   * Flash the firmware located in the `/firmware` directory. // to do
   * Run the nRF Logger application or dedicated logger: `ble-sht41.html`
   * Note! Android may block long-term Web-BLE connections through an HTML page. Personally, I managed to maintain an active connection and log for the planned 24 hours. This was likely due to connecting the smartphone to a power source—in this configuration, Android can maintain the BLE connection longer. If you encounter issues maintaining a long connection via `ble.sht41.html`, use the nRF Connect + nRF Logger apps, then prepare the log file for further analysis using the Python script provided in the service directory.
   * Data analysis ...

---

## Results 

... 