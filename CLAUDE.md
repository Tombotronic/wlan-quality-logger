# Cardputer Adv

Project for the [M5Stack Cardputer Adv](https://docs.m5stack.com/en/core/Cardputer-Adv) — a pocket ESP32-S3 dev board with keyboard, screen, mic/speaker, and IMU.

## Hardware

- **SoC:** ESP32-S3FN8, Xtensa LX7 dual-core @ 240MHz, 8MB flash, WiFi/BLE
- **Display:** 1.14" 240x135, ST7789V2 controller
- **Keyboard:** 56-key matrix (4x14), via TCA8418RTWR I2C keyboard controller
- **IMU:** BMI270 (6-axis), on I2C
- **Audio:** ES8311 codec, NS4150B amp + 8Ω/1W speaker, MEMS mic, 3.5mm jack (disables speaker when plugged in)
- **Storage:** microSD slot (SPI) — see [SD card](#sd-card)
- **IR:** IR emitter (G44)
- **Battery:** 1750mAh Li-ion, ADC on G10
- **Expansion:** HY2.0-4P Grove port (GND/5V/G2/G1), EXT 2.54-14P bus (UART/I2C/SPI/power)
- **Body:** 84x54x19.6mm, 81g, magnetic back, LEGO-compatible holes

### Pinout (Stamp-S3A GPIOs)

| Peripheral | Pins |
|---|---|
| LCD | BL=G38, RST=G33, RS=G34, DAT=G35, SCK=G36, CS=G37 |
| Audio (ES8311) | SDA=G8, SCL=G9, SCLK=G41, ASDOUT=G46, LRCK=G43, DSDIN=G42 |
| IMU (BMI270) | SDA=G8, SCL=G9 |
| Keyboard (TCA8418) | SDA=G8, SCL=G9, INT=G11 |
| microSD | CS=G12, MOSI=G14, CLK=G40, MISO=G39 |
| IR TX | G44 |
| Battery ADC | G10 |

Note: audio, IMU, and keyboard all share the same I2C bus (G8/G9).

## Charging

- **Port:** USB-C
- Power switch must be set to **ON** while charging
- No charging LED indicator or documented charge time/current — not detailed in the official docs

## Factory reset

**Preferred: restore from local backup.** The exact factory flash (as shipped, before any of our own firmware was written) is backed up at `backups/factory-backup-V0.9-36-ge824a76.bin` (8MB, full flash dump via `esptool`). This is the specific build that was on the device when we started — project `cardputer-adv` V0.9-36-ge824a76, ESP-IDF v5.4.2, launcher v2.0.0, compiled Oct 21 2025.

Restore with:
```
esptool.py --port /dev/cu.usbmodem2101 write_flash 0x0 backups/factory-backup-V0.9-36-ge824a76.bin
```

**Fallback (if the local backup is lost):** M5Stack's Windows-only Easyloader (`.exe`) is linked on the docs page, but on macOS use **M5Burner** instead:

1. Download [M5Burner for macOS (v3.0, x64)](https://m5burner-cdn.m5stack.com/app/M5Burner-v3-mac-x64.dmg)
2. Open it, search for "Cardputer-Adv" in its device catalog (pulls firmware, including the factory demo, from M5Stack's cloud)
3. Connect the device — it enumerates as a "USB JTAG/serial debug unit" (`/dev/cu.usbmodem*`)
4. Select the board, hit burn — M5Burner handles entering download mode itself

Manual download-mode entry (if not using M5Burner): flip the side power switch to **OFF**, hold **G0**, power on, then release G0.

Factory firmware source (if building it yourself instead): [M5Cardputer-UserDemo, CardputerADV branch](https://github.com/m5stack/M5Cardputer-UserDemo/tree/CardputerADV).

## SD card

Card in use: **SanDisk Ultra 64GB microSDXC** (C10/U1/A1). The slot runs in **SPI mode** (CS=G12, MOSI=G14, CLK=G40, MISO=G39), and SPI-mode exFAT support in Arduino's `SD` library is unreliable — SDXC cards ship exFAT by default, so **reformat to FAT32** before use.

Steps on macOS:
1. Insert the card (via adapter/reader), then run `diskutil list` to find its disk identifier (e.g. `/dev/disk4`) — confirm size/name matches before doing anything else
2. Reformat:
   ```
   diskutil eraseDisk FAT32 CARDPUTER MBRFormat /dev/diskN
   ```
   (replace `diskN` with the confirmed identifier — **never guess this**, wrong disk = data loss)
3. Eject cleanly (`diskutil eject /dev/diskN`) before removing

A 32GB microSDHC card (e.g. SanDisk Ultra 32GB) would skip this step entirely — SDHC ships FAT32 natively.

## Dev platforms

- **Arduino IDE** — [M5Cardputer library](https://github.com/m5stack/M5Cardputer)
- **UiFlow2** — block/MicroPython, official graphical tool
- **ESP-IDF** — [factory firmware source](https://github.com/m5stack/M5Cardputer-UserDemo/tree/CardputerADV)
- **PlatformIO** — supported

Default to **Arduino (C++) with the M5Cardputer library** unless told otherwise — it has the most examples and direct hardware support for this board. Since C++ isn't Thomas's preferred language, keep code simple and well-explained; lean on the library's high-level APIs rather than raw register/driver code.

## Reference docs

- [Product page / specs](https://docs.m5stack.com/en/core/Cardputer-Adv)
- [Schematic (Cardputer Adv)](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1178/Sch_M5CardputerAdv_v1.0_2025_06_20_17_19_58.pdf)
- [Schematic (Stamp-S3A module)](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1150/Sch_StampS3_v0.3.3.pdf)
- [Hardware design files](https://github.com/m5stack/M5_Hardware)
