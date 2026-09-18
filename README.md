# WLAN Quality Logger

An Arduino (C++) sketch for the [M5Stack Cardputer Adv](https://docs.m5stack.com/en/core/Cardputer-Adv) — a pocket ESP32-S3 dev board with keyboard, screen, mic/speaker, and IMU.

Joins your home WiFi, logs signal strength (RSSI) to the SD card every minute with a real timestamp (NTP), and serves a live + historical chart over the network.

WiFi credentials are entered on-device via the keyboard (scan and pick an SSID, or type one manually) and stored in flash — never hardcoded. A "Forget WiFi" button on the web dashboard wipes them so you can re-enter new ones.

Code: [`apps/wifi_logger`](apps/wifi_logger)

![On-device screen showing RSSI in dBm and battery percentage](docs/device.jpg)

![Web dashboard showing RSSI chart, battery, and Forget WiFi button](docs/dashboard.png)

## Flashing a precompiled release

No Arduino IDE needed. Grab the latest `.bin` from [Releases](../../releases), then:

```
pip install esptool   # if you don't already have it
esptool.py --chip esp32s3 --port /dev/cu.usbmodemXXXX write_flash 0x0 wlan-quality-logger-vX.Y.Z.bin
```

(On Windows/Linux the port looks like `COM3` or `/dev/ttyACM0`.) Insert a FAT32-formatted microSD card before powering on — the sketch halts if it can't mount one. First boot walks you through WiFi setup on the device's own keyboard.

## Hardware

- **SoC:** ESP32-S3FN8, Xtensa LX7 dual-core @ 240MHz, 8MB flash, WiFi/BLE
- **Display:** 1.14" 240x135, ST7789V2 controller
- **Keyboard:** 56-key matrix (4x14), via TCA8418RTWR I2C keyboard controller
- **Storage:** microSD slot (SPI, must be FAT32-formatted)
- **Battery:** 1750mAh Li-ion, ADC on G10

## Dev platform

Arduino IDE (or `arduino-cli`) with the [M5Cardputer library](https://github.com/m5stack/M5Cardputer), which pulls in M5Unified/M5GFX. Board: `esp32:esp32:esp32s3`.
