# ESP32-OLED-Weather-Clock

ESP32 128×64 OLED I2C Smart Desktop Weather Clock. Supports Chinese display, Lunar Calendar, Solar Terms, QWeather V1 API for current weather + 7-day forecast, Button-based WiFi configuration, Offline cache, Night screen-off power saving, and Hourly chime.

---

[JWT No-Dial Version Special Instructions](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT_EN.md)

[English](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_EN.md)

[简体中文](https://github.com/caocq3012/esp32-clock-weather/blob/main/README.md)

[日本語](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JP.md)

## 📌 Project Introduction

An OLED desktop clock based on the ESP32 development board, compatible with SSD1306 128×64 I2C OLED screens.

Main Features:

- **Automatic NTP Time Sync**: Syncs immediately on boot + periodic refresh, continues to run offline.
- **Lunar Calendar + Solar Terms**: Fetches lunar date and solar terms online via apihz.cn.
- **IP Auto-Location**: Gets latitude, longitude, and city name via ip9.com.cn.
- **QWeather V1 API**: Current weather + 7-day forecast, with manual gzip decompression (miniz).
- **WiFi Configuration**: Visual network scanning, character input for password, XOR encrypted storage in Preferences.
- **4-Button Interaction**: Page switching, menu selection, WiFi password input, long-press confirmation.
- **Hourly Chime**: Can be toggled on/off.
- **Automatic Night Screen-Off**: Screen off from 21:00 to 07:00. Press DOWN to wake screen for 10 seconds; stays on during the day.
- **Offline Cache**: Reads historical weather cache when disconnected.
- **Hand-drawn Weather Icons**: Sunny/Cloudy/Rain/Thunder/Snow/Fog/Haze/Sandstorm, etc.
- **Serial Heartbeat Log**: For easy debugging.

---

## 🧰 Hardware List

| Hardware   | Model/Parameters                                                                                                                                                     |
| ---------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| MCU        | ESP32 Development Board (Any model, Flash ≥ 4MB)                                                                                                                     |
| Screen     | SSD1306, 128×64, I2C, Address `0x3C` Taobao Link (Login required): https://item.taobao.com/item.htm?id=767205498137&skuId=5440471327277 , select 4-pin IIC OLED LCD) |
| Buttons ×4 | Tactile switches                                                                                                                                                     |
| Buzzer     | Active or Passive, connected to GPIO 23                                                                                                                              |

Tips: The buzzer keeps ringing when connected and I don't know why. Any experts please help. If you don't know, better not to connect it first.

### Pin Definitions

| Peripheral       | GPIO    |
| ---------------- | ------- |
| OLED SDA         | GPIO 19 |
| OLED SCL         | GPIO 18 |
| Buzzer BEEP      | GPIO 23 |
| KEY_UP   Button  | GPIO 27 |
| KEY_DOWN Button  | GPIO 26 |
| KEY_LEFT Button  | GPIO 25 |
| KEY_RIGHT Button | GPIO 33 |

> ⚠️ OLED power supply recommended 3.3V, do not connect 5V to avoid burning the screen. Default I2C address is `0x3C`; if the screen is black, try `0x3D`.

---

## 📦 Dependencies

**Arduino IDE → Library Manager to install the following libraries:**

| Library Name            | Author           | Purpose                                 |
| ----------------------- | ---------------- | --------------------------------------- |
| `Adafruit GFX Library`  | Adafruit         | Graphics drawing base library           |
| `Adafruit SSD1306`      | Adafruit         | OLED screen driver                      |
| `U8g2_for_Adafruit_GFX` | olikraus         | OLED Chinese fonts (GB2312)             |
| `NTPClient`             | Fabrice Weinberg | NTP time sync                           |
| `ArduinoJson`           | Benoit Blanchon  | JSON parsing (**Requires 6.x version**) |

**Included in project files:**

- `miniz.c` / `miniz.h` —— gzip decompression (manual tinfl implementation)

**Place in project directory (same level as `.ino`):**

esp32_clock_and_weather/
├── sketch_sep25a.ino
├── miniz.c
└── miniz.h

---

## ⚙️ Compilation Configuration

Arduino IDE → **Tools → Partition Scheme** → Select:

Huge APP (3MB No OTA/1MB SPIFFS)

**Otherwise compilation will fail** (Chinese + SSL + miniz exceeds the default 1.3MB app partition).

---

## 🔑 User Configuration

Open the `.ino` file, find the "User Configuration Area", and fill in your account information:

// QWeather (https://console.qweather.com/)
const char* QWEATHER_API_KEY  = "Fill in QWeather api-key";
const char* QWEATHER_API_HOST = "Fill in QWeather api-host";

// apihz (https://www.apihz.cn/)
const char* LUNAR_API_ID  = "Fill in apihz developer ID";
const char* LUNAR_API_KEY = "Fill in apihz developer API-key";

### QWeather Configuration

1. Register at https://console.qweather.com/
2. "Project Management" → Create Project
3. "Credentials" → Generate API Key
4. "Settings" → Check API Host

### apihz Configuration

1. Register at https://www.apihz.cn/
2. Find your **Developer ID** and **API Key** in the Personal Center
3. Fill them into the code

---

## 🎮 Button Operations

| Button                    | Clock/Weather Page     | Menu Page        | WiFi Scan      | Password Input        |
| ------------------------- | ---------------------- | ---------------- | -------------- | --------------------- |
| **LEFT**                  | Switch to Clock Page   | Return           | Return to Menu | Backspace             |
| **RIGHT**                 | Switch to Weather Page | Enter Option     | Select SSID    | Add Character         |
| **UP**                    | Enter Menu             | Move Option Up   | List Up        | Character +1          |
| **DOWN**                  | —                      | Move Option Down | List Down      | Character -1          |
| **Long Press LEFT/RIGHT** | Toggle Hourly Chime    | —                | —              | Long Press to Connect |

---

## 🔄 Data Update Frequency

| Data                         | Frequency                          |
| ---------------------------- | ---------------------------------- |
| NTP Time                     | Once on boot + Every 5 minutes     |
| Weather (Current + 7 Days)   | Once on boot + Every 10 minutes    |
| Lunar Calendar + Solar Terms | Once a day (Refreshes at midnight) |

---

## 📂 Version Description & Download

All versions are integrated into the `main` branch, **no need to switch branches**. Please click into the corresponding folder to download based on your needs:

- 📁 **`Dial Version/sketch_sep25a`** —— Complete version with dial UI

- 📁 **`No-Dial Version/sketch_sep25a`** —— Recommended, as the dial display is a bit crowded

- 📁 **`No-Dial JWT Version/sketch_sep25a`** —— Advanced security version (complex configuration) [JWT No-Dial Special Instructions](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT_EN.md)

**💡 Special Instructions:**

- **No-Dial Version (including Dial Version)**: Uses standard API Key to access QWeather, simple configuration, suitable for beginners.

- **No-Dial JWT Version**: Uses Ed25519 algorithm to generate JWT to access QWeather, greatly improved security, but complex configuration.

  - ⚠️ **Note**: The JWT version involves many low-level encryption dependency files. After downloading, please be sure to read [JWT No-Dial Special Instructions](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT_EN.md) for configuration.

**🔑 Quick Configuration Guide (for Standard Version):**  
After downloading the code, open the `.ino` file and fill in your WiFi, QWeather, and apihz information directly in the **"User Configuration Area"** at the beginning to compile and run. If you have higher security needs, you can also create a `key.h` file in the same directory to fill in the configuration, and the program will automatically read `key.h` first.

## ⚠️ Notes

- **Weather features only support use within China** (depends on ip9.com.cn location + QWeather domestic API)
- **Lunar calendar feature depends on apihz API**, request frequency is low (once a day), free quota is sufficient
- **API Key: Do not disclose**, otherwise others will use up your quota
- **Do not push WiFi passwords or API Keys in plain text to GitHub**

---

- 📁 **`text/sketch_oct24a`** —— Bonus: Pure hardware test program (used to test if the OLED and buttons are working properly)

---

## 🙏 Acknowledgements

- Weather Data: [QWeather](https://www.qweather.com/)
- Lunar API: [apihz](https://www.apihz.cn/)
- IP Location: [ip9.com.cn](http://ip9.com.cn/)
- gzip Decompression: [miniz](https://github.com/richgel999/miniz) (richgel999)
- Chinese Font Rendering: [U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX) (olikraus)
- Time Sync: [NTPClient](https://github.com/arduino-libraries/NTPClient) (Fabrice Weinberg)

---

## 📄 License

This project is open-sourced under the [MIT License](LICENSE), Copyright (c) 2026 gaocq.

### Third-Party Components

This project (except the JWT No-Dial Version) uses the following third-party open-source components, each following its original license:

- **[miniz](https://github.com/richgel999/miniz)** —— MIT License
  Copyright 2013-2014 RAD Game Tools and Valve Software
  Copyright 2010-2014 Rich Geldreich and Tenacious Software LLC
- **Adafruit GFX / SSD1306**: BSD
- **ArduinoJson**: MIT
- **NTPClient**: MIT
- **U8g2_for_Adafruit_GFX**: MIT
- **QWeather API**: Subject to QWeather Terms of Service
- **apihz.cn Lunar API**: Subject to apihz Terms of Service

---

## ⭐ If you find this project useful, welcome to click Star

---
