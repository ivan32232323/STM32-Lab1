# STM32 ЛР1 — bring-up (форма B, Wokwi) · Motion-Mirroring Teleoperator

Прошивка на STM32 HAL для **Nucleo-C031C6** у симуляторі Wokwi: USART2 (printf), I²C1 + MPU-6050,
ADC (потенціометр), TIM3 PWM (2 серво), GPIO (кнопка, LED).

## Швидкий старт

```bash
make image      # один раз: Docker-образ з тулчейном
make build      # firmware/build/firmware.elf|hex|bin
make flash      # підказка: перезапуск Wokwi з новим ELF
make monitor    # підказка: Serial Monitor Wokwi
```
Windows без make: `build.bat`.

Далі у VS Code: **F1 → Wokwi: Start Simulator** (використовує `wokwi.toml` і `diagram.json`).

## Структура
```
docker/Dockerfile          тулчейн-образ (ubuntu 22.04 + arm-none-eabi-gcc)
firmware/                  CubeMX-проєкт (firmware.ioc, Core/, Drivers/, Makefile)
diagram.json, wokwi.toml   схема і конфіг симулятора
docs/PRD.md                вимоги до продукту
docs/hardware-components.md датчики, піни, регістри, посилання на datasheet
docs/report.md             звіт (джерело для report.pdf)
docs/screenshots/          скриншоти CubeMX / Wokwi / logic analyzer
Makefile                   обгортка над Docker
```

## Розводка
| Сигнал | Пін | Периферія |
|---|---|---|
| VCP TX/RX | PA2 / PA3 | USART2 115200 8N1 |
| MPU-6050 SCL/SDA | PB8 (D15) / PB9 (D14) | I2C1, 0x68 |
| Потенціометр | PA0 (A0) | ADC1_IN0 |
| Серво pan / tilt | PA6 (D12) / PA7 (D11) | TIM3_CH1 / CH2, 50 Гц |
| Кнопка MODE | PA10 (D2) | GPIO in, pull-up |
| LED LD4 | PA5 (D13) | GPIO out |
