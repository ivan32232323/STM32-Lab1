# Апаратні компоненти (ЛР1, форма B — Wokwi)

Платформа: **Nucleo-C031C6** (MCU STM32C031C6T6, Cortex-M0+, 48 МГц, 32 КБ Flash, 12 КБ RAM).

> Чому не F411RE/F446RE: Wokwi симулює з лінійки STM32 лише Nucleo-C031C6, Nucleo-L031K6 і Blue Pill
> ([Wokwi diagram format][wokwi-diag]). Тулчейн і HAL ті самі, змінюються лише `-mcpu=cortex-m0plus`
> (без FPU), стартап і лінкер-скрипт. На реальній Nucleo-F411RE ті самі піни D11/D12/D14/D15/A0
> мають ті самі alternate functions (I2C1, TIM3, ADC1), тож перенесення зводиться до перегенерації в CubeMX.

## 1. Зведена таблиця

| Компонент | Інтерфейс | Пін Nucleo (MCU) | AF / канал | Живлення | Ідентифікація | Джерело |
|---|---|---|---|---|---|---|
| ST-Link VCP (UART Logger) | UART, 115200 8N1 | PA2 TX, PA3 RX | USART2, AF1 | — | рядок `hello from STM32` | [DS-C031] табл. «Alternate functions», [UM-Nucleo] розд. «Virtual COM port» |
| **MPU-6050** (GY-521) | **I²C**, 100 кГц (макс. 400 кГц) | D15 = PB8 SCL, D14 = PB9 SDA | I2C1, AF6 | 3.3 В (VDD 2.375–3.46 В) | `WHO_AM_I` (0x75) = **0x68** | [PS-MPU] с. 15 (I²C address, 100/400 кГц), с. 33–35 (I²C protocol); [RM-MPU] с. 45 (WHO_AM_I) |
| **Потенціометр** 10 кОм | **ADC**, 12 біт, 0…3.3 В | A0 = PA0 | ADC1_IN0 | 3.3 В | — (аналоговий) | [DS-C031] табл. «Pin definitions», [RM0490] розд. ADC |
| **Серво SG90/SG92R** ×2 | **PWM** 50 Гц, 1–2 мс | D12 = PA6 (pan), D11 = PA7 (tilt) | TIM3_CH1/CH2, AF1 | **5 В** (4.8–6 В) | — | [SG90] |
| **Кнопка MODE** | **GPIO** вхід, pull-up | D2 = PA10 | — | — | — | — |
| Світлодіод LD4 | GPIO вихід | D13 = PA5 | — | — | — | [UM-Nucleo] |
| Logic Analyzer (Wokwi) | sniff D0–D3 | D15, D14, D12, PA2 | — | — | — | [Wokwi-LA] |

Мінімум за методичкою: 3 компоненти з різними інтерфейсами. Тут їх **чотири: I²C, ADC, PWM і GPIO**, плюс UART як канал логів.

## 2. MPU-6050 — 6-DOF IMU (InvenSense / TDK)

* **Інтерфейс:** I²C slave. 7-бітна адреса `0b110100X`, де X = рівень на піні AD0.
  AD0 = GND → **0x68**, AD0 = VCC → 0x69 ([PS-MPU] с. 15, табл. 6.4 «I²C ADDRESS»). У схемі AD0 → GND.
* **Швидкість:** Standard 100 кГц / Fast 400 кГц ([PS-MPU] с. 15, «I²C Operating Frequency»). Беремо 100 кГц,
  `Timing = 0x20303E5D` @ 48 МГц (значення з CubeMX).
* **Піни:** VCC, GND, SCL, SDA, XDA/XCL (допоміжна шина, не використовуємо), AD0, INT.
  Плата GY-521 має LDO 3.3 В і підтяжки 4.7 кОм на SCL/SDA.
* **Регістри для bring-up** ([RM-MPU]):
  * `0x75 WHO_AM_I` → 0x68 — ідентифікація ([RM-MPU] с. 45, розд. 4.32);
  * `0x6B PWR_MGMT_1` ← 0x00 — вийти зі сну, після reset чип спить ([RM-MPU] с. 40, розд. 4.28);
  * `0x3B…0x40 ACCEL_XOUT_H…ACCEL_ZOUT_L` — сирі дані акселерометра, ±2 g = 16384 LSB/g ([RM-MPU] с. 29, розд. 4.17);
  * `0x43…0x48 GYRO_*` — гіроскоп, ±250 °/с = 131 LSB/(°/с) (розд. 4.19), знадобиться у ЛР2 для fusion.
* **Живлення:** 3.8 мА (gyro + accel), 3.9 мА з DMP; VDD 2.375–3.46 В ([PS-MPU] с. 14, табл. 6.3). Живимо від 3V3 Nucleo,
  зовнішнє джерело не потрібне.
* **Навіщо в проєкті:** вимірює нахил кисті оператора (pitch/roll) → FR-01, FR-02.

## 3. Потенціометр (аналоговий вхід)

* **Інтерфейс:** ADC1, канал IN0 (PA0), 12 біт → 0…4095 при 0…VDDA = 3.3 В.
* **Час вибірки:** 39.5 циклів (`ADC_SAMPLETIME_39CYCLES_5`), з запасом для джерела 10 кОм.
* **Живлення:** ~0.33 мА через доріжку 10 кОм від 3.3 В.
* **Навіщо:** ручне керування серво (режим MANUAL) і калібрування нуля / коефіцієнта → FR-05, FR-06.
  На реальному залізі його замінює фоторезистор GM5528 у дільнику.

## 4. Сервоприводи SG90 / SG92R

* **Інтерфейс:** PWM, період 20 мс (50 Гц), імпульс 1.0 мс = −90°, 1.5 мс = 0°, 2.0 мс = +90° ([SG90]).
* **Налаштування таймера:** TIM3, PSC = 47 → 1 МГц (1 тік = 1 мкс), ARR = 19999 → 20 мс. CCR = ширина імпульсу в мкс.
* **Живлення:** 4.8–6 В. Струм холостого ходу ~10 мА, під навантаженням до **~360 мА** (stall) [SG90].
  ⚠ На реальній платі **не живити від 3V3/5V Nucleo**, бо струм ST-Link USB обмежений.
  Потрібне окреме джерело 5 В і спільний GND. У Wokwi 5V-пін досить.
* **Навіщо:** «віддалена рука» pan/tilt → FR-01, FR-02, FR-04.

## 5. Кнопка MODE

* **Інтерфейс:** GPIO вхід, внутрішній pull-up. Натиснута = 0. Антидребезг 20 мс програмно.
* **Навіщо:** перемикання режимів MANUAL / MIRROR (у ЛР2 + RECORD / PLAYBACK) → FR-07.

## 6. Перевірка шин

`HAL_I2C_IsDeviceReady()` для адрес 0x08…0x77 (I²C-scan) знаходить пристрій **0x68**. У Wokwi Logic Analyzer
на D0 (SCL) видно тактові імпульси 100 кГц, на D2 — PWM 50 Гц зі шпаруватістю 1–2 мс.

## Джерела

* [PS-MPU] InvenSense, *MPU-6000 and MPU-6050 Product Specification*, PS-MPU-6000A-00, Rev. 3.4. https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
* [RM-MPU] InvenSense, *MPU-6000 and MPU-6050 Register Map and Descriptions*, RM-MPU-6000A-00, Rev. 4.2. https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
* [DS-C031] STMicroelectronics, *STM32C031x4/x6 datasheet*, DS13867. https://www.st.com/resource/en/datasheet/stm32c031c6.pdf
* [RM0490] STMicroelectronics, *STM32C0x1 reference manual*, RM0490. https://www.st.com/resource/en/reference_manual/rm0490-stm32c0x1-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
* [UM-Nucleo] STMicroelectronics, *STM32 Nucleo-64 boards (MB1717)*, UM3061 (NUCLEO-C031C6). https://www.st.com/en/evaluation-tools/nucleo-c031c6.html
* [SG90] TowerPro, *SG90 Micro Servo datasheet*. http://www.ee.ic.ac.uk/pcheung/teaching/DE1_EE/stores/sg90_datasheet.pdf
* [wokwi-diag] Wokwi, *diagram.json File Format*. https://docs.wokwi.com/diagram-format
* [Wokwi-LA] Wokwi, *Logic Analyzer*. https://docs.wokwi.com/parts/wokwi-logic-analyzer

> Сторінки вказано за PS-MPU-6000A-00 Rev 3.4 і RM-MPU-6000A-00 Rev 4.2. Вирізки цих сторінок лежать у `docs/datasheets/`.
