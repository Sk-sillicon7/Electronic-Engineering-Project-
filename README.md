
# 🛠️ ESP32 16x2 I2C Smart Display Firmware & Docs

This repository contains the working firmware code, hardware wiring matrix, and step-by-step setup instructions to stream text from the Arduino Serial Monitor onto a 16x2 LCD screen using an ESP32 and an I2C backpack interface.

---

## 📌 How It Works (Technical Overview)
* *No Complex Protocol Overhead:* Direct implementation using standard Wire.h I2C communication on default ESP32 pins.
* *Boot Sequence Check:* The display triggers a quick splash screen message (ESP32 Ready -> Serial->LCD) on startup so you know the hardware initialized properly before data streaming begins.
* *Smart Row Buffering:* Automatically handles and cleans incoming serial character arrays, splits strings across the 16x2 grid (up to 32 characters total), and triggers a screen clear command right before new data hits the display to prevent overlapping text bugs.

---

## 🔌 Hardware Setup & Connections

### 📦 Component Breakdown
* *Microcontroller:* ESP32 Development Board (NodeMCU / DoIT 30-pin variant).
* *Display Module:* Standard 16x2 Alphanumeric LCD with a pre-soldered PCF8574 I2C adapter board.
* *Default Hex Addresses:* Usually pre-configured to 0x27 or 0x3F depending on the backpack manufacturer.

### 🧵 Pin Mapping Table
| From LCD (I2C Backpack) | To ESP32 Development Board | Power & Signal Notes |
| :--- | :--- | :--- |
| *VCC* | 5V / VIN (or 3.3V) | Check module datasheet for backlight voltage |
| *GND* | GND | Common ground loop |
| *SDA* | GPIO 21 | Dedicated Hardware I2C Serial Data line |
| *SCL* | GPIO 22 | Dedicated Hardware I2C Serial Clock line |

---

## 🚀 Step-by-Step Deployment Guide

1. *Environment Config:* Open your *Arduino IDE* and make sure you have the official esp32 board manager package installed (by Espressif).
2. *Library Dependency:* Go to Tools -> Manage Libraries and install the standard LiquidCrystal_I2C library by Frank de Brabander.
3. *Flashing the Chip:* Open the ESP32_LCD_Serial.ino sketch file from this repo, plug in your board via micro-USB, select the right COM port, and hit Upload.
4. *Testing Data Streams:* Once flashed, open up your Serial Monitor, change the baud rate setting to 115200, and set the line ending dropdown menu to Both NL & CR. Type any text in the input bar, hit enter, and watch it render live on the LCD hardware.

---

## 💼 Technical Services & Professional Engineering
This project is an example of my approach to clean hardware documentation and functional prototyping. I specialize in system integration, Bill of Materials (BOM) cost reduction, and embedded electronics troubleshooting.

📬*Looking for custom circuit design, component sourcing, or product documentation?*  
Let's talk business: **[Hire Me on Upwork](https://www.upwork.com/freelancers/~0147947d9c4070f5a5?mp_source=share)**
