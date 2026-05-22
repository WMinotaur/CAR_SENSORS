# CAR_SENSORS

Firmware repository for a car sensor project: an **STM32 sensor node** and an **ESP32 CAN/TWAI** component.

GitHub: https://github.com/WMinotaur/CAR_SENSORS

## Repository layout
- `SensorNode/stm32/` – STM32 application (C/CMake; project configuration files like `prj.conf`, `west.yml`, etc.)
- `esp/ESP_CAN/` – ESP32 project (ESP-IDF) for CAN communication using TWAI

## Requirements (high level)
- Build tools for the STM32 part (CMake + toolchain / environment matching `SensorNode/stm32/` config)
- ESP-IDF for `esp/ESP_CAN/`
- (Optional) Docker / devcontainer configs are included to simplify environment setup

## Build (quick guide)
- **STM32:** go to `SensorNode/stm32/` and build using the provided CMake/project configuration.
- **ESP32:** go to `esp/ESP_CAN/` and use the standard ESP-IDF workflow (e.g. `idf.py build/flash/monitor`).

## Status
Work in progress — hardware details (pinout, CAN IDs, sensors) are defined in the source code and `Kconfig`/`sdkconfig`.
