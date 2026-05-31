# ESP32 I2C Smart Display

Part of the [Electronics Engineering Projects Portfolio](../README.md) by **Shafia Khan**.

A lightweight ESP32 firmware project that displays Serial Monitor input on a 16×2 I2C LCD. See the root portfolio README for the full project list.

---

## Project Overview

The ESP32 acts as a serial-to-display bridge: text sent from the Arduino Serial Monitor is parsed, sanitized, and rendered on an HD44780-compatible LCD with a PCF8574 I2C backpack. Each new line clears the screen before updating both rows (up to 32 characters).

---

## Key Features

- I2C LCD on GPIO 21 (SDA) and GPIO 22 (SCL)
- Welcome screen on boot (`ESP32 Ready` / `Serial->LCD`)
- Live Serial input at 115200 baud
- Automatic LCD clear on each new message
- Two-row layout with input sanitization and serial echo

---

## Hardware Components

| Component | Notes |
|-----------|--------|
| ESP32 development board | USB programming and power |
| 16×2 LCD + I2C backpack | Typical address `0x27` or `0x3F` |
| Jumper wires | SDA, SCL, VCC, GND |

---

## Pin Mapping

| Signal | ESP32 GPIO |
|--------|------------|
| **SDA** | **21** |
| **SCL** | **22** |
| VCC | 3.3 V or 5 V (per module) |
| GND | GND |

---

## Software Requirements

- Arduino IDE with **esp32** board support (Espressif)
- Library: **LiquidCrystal I2C** (Frank de Brabander)

---

## Setup Instructions

1. Wire SDA → GPIO 21, SCL → GPIO 22, power and ground.
2. Install ESP32 core and LiquidCrystal I2C library.
3. Open `ESP32_LCD_Serial.ino`, select your ESP32 board, and upload.
4. Serial Monitor: **115200** baud, line ending **Newline** or **Both NL & CR**.
5. If the display is blank, set `LCD_I2C_ADDRESS` to `0x3F` in the sketch.

---

## Author

**Shafia Khan** — [Portfolio](../README.md)
