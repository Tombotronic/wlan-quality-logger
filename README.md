# Cardputer Adv

Arduino (C++) sketches for the [M5Stack Cardputer Adv](https://docs.m5stack.com/en/core/Cardputer-Adv) — a pocket ESP32-S3 dev board with keyboard, screen, mic/speaker, and IMU.

## Apps

| App | Description |
|---|---|
| [`wifi_logger`](apps/wifi_logger) | Joins your home WiFi, logs signal strength (RSSI) to the SD card every minute with a real timestamp (NTP), and serves a live + historical chart over the network. WiFi credentials are entered on-device via the keyboard (scan and pick an SSID, or type one manually) and stored in flash — never hardcoded. A "Forget WiFi" button on the web dashboard wipes them. |
| [`webui_demo`](apps/webui_demo) | The Cardputer creates its own WiFi hotspot and serves a page showing live accelerometer/gyro readings from the onboard BMI270. |
| [`imu_demo`](apps/imu_demo) | Shows live accelerometer + gyro values from the onboard BMI270 directly on the device screen. |
| [`sd_test`](apps/sd_test) | Mounts the microSD card, writes a test file, reads it back, and shows the result on screen. |

## Hardware

- **SoC:** ESP32-S3FN8, Xtensa LX7 dual-core @ 240MHz, 8MB flash, WiFi/BLE
- **Display:** 1.14" 240x135, ST7789V2 controller
- **Keyboard:** 56-key matrix (4x14), via TCA8418RTWR I2C keyboard controller
- **IMU:** BMI270 (6-axis), on I2C
- **Audio:** ES8311 codec, NS4150B amp + 8Ω/1W speaker, MEMS mic, 3.5mm jack
- **Storage:** microSD slot (SPI, must be FAT32-formatted)
- **IR:** IR emitter (G44)
- **Battery:** 1750mAh Li-ion, ADC on G10

See [`CLAUDE.md`](CLAUDE.md) for the full pinout, factory-reset instructions, and SD card setup notes.

## Dev platform

Arduino IDE (or `arduino-cli`) with the [M5Cardputer library](https://github.com/m5stack/M5Cardputer), which pulls in M5Unified/M5GFX. Board: `esp32:esp32:esp32s3`.
