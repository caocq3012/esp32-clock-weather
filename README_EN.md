# ESP32-OLED-Weather-Clock

ESP32 128×64 OLED I2C smart desktop weather clock. Supports Chinese display, lunar calendar, solar terms, QWeather V1 API automatic fetching of current weather + 7-day forecast, button-based WiFi provisioning, offline caching, nighttime screen-off power saving, and hourly buzzer time announcement.

---

[JWT No-Dial Version Special Notes](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT.md)

## 📌 Project Introduction

An OLED desktop clock based on the ESP32 development board, adapted for SSD1306 128×64 I2C OLED screens.

Main features:

- **Automatic NTP network time sync**: Supports immediate sync on boot + periodic refresh, can continue running offline
- **Lunar calendar + solar terms**: Online retrieval of lunar month/day and solar term information via apihz.cn
- **IP automatic geolocation**: Obtains latitude/longitude and city name via ip9.com.cn
- **QWeather V1 API**: Current weather + 7-day forecast, with manual gzip decompression (miniz)
- **WiFi provisioning**: Visual network selection from scan list, password character input, XOR-encrypted storage in Preferences
- **Four-button interaction**: Page switching, menu selection, WiFi password input, long-press confirmation
- **Hourly buzzer time announcement**: Supports on/off toggle
- **Automatic nighttime screen-off**: Screen off from 21:00 to 07:00, press the down key to turn on the screen for 10 seconds, stays on during the day
- **Offline caching**: Reads historical weather cache when disconnected
- **Hand-drawn weather icons**: Sunny/cloudy/rain/thunder/snow/fog/haze/sandstorm, etc.
- **Serial heartbeat logging**: For easy debugging

---

## 🧰 Hardware List

| Hardware   | Model/Parameters                                                                                                                                                                                 |
| ---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| MCU        | ESP32 development board (any model, Flash ≥ 4MB)                                                                                                                                                 |
| Screen     | SSD1306, 128×64, I2C, address `0x3C` Taobao link (login required): https://item.taobao.com/item.htm?id=767205498137&skuId=5440471327277    , select 4-pin IIC OLED LCD screen for specification) |
| Buttons ×4 | Tactile switches                                                                                                                                                                                 |
| Buzzer     | Active or passive, connected to GPIO 23                                                                                                                                                          |

tips: I don't know why the buzzer keeps ringing when connected. If any expert could take a look, and if not, it's best not to connect it for now

### Pin Definitions

| Peripheral             | GPIO    |
| ---------------------- | ------- |
| OLED SDA               | GPIO 19 |
| OLED SCL               | GPIO 18 |
| Buzzer BEEP            | GPIO 23 |
| KEY_UP Up button       | GPIO 27 |
| KEY_DOWN Down button   | GPIO 26 |
| KEY_LEFT Left button   | GPIO 25 |
| KEY_RIGHT Right button | GPIO 33 |

> ⚠️ OLED power supply recommended 3.3V, do not connect 5V to avoid burning the screen. I2C address defaults to `0x3C`; if the screen is black, try changing it to `0x3D`.

---

## 📦 Dependencies

**Arduino IDE → Install the following libraries via Library Manager:**

| Library Name            | Author           | Purpose                                 |
| ----------------------- | ---------------- | --------------------------------------- |
| `Adafruit GFX Library`  | Adafruit         | Graphics drawing foundation library     |
| `Adafruit SSD1306`      | Adafruit         | OLED screen driver                      |
| `U8g2_for_Adafruit_GFX` | olikraus         | OLED Chinese font (GB2312)              |
| `NTPClient`             | Fabrice Weinberg | NTP time sync                           |
| `ArduinoJson`           | Benoit Blanchon  | JSON parsing (**requires version 6.x**) |

**Bundled with the project files:**

- `miniz.c` / `miniz.h` —— gzip decompression (manually implemented tinfl decompression)

**Place in the project directory (same level as `.ino`):**

```
esp32_clock_and_weather/
├── sketch_sep25a.ino
├── miniz.c
└── miniz.h
```

---

## ⚙️ Compilation Configuration

Arduino IDE → **Tools → Partition Scheme** → Select:

```
Huge APP (3MB No OTA/1MB SPIFFS)
```

**Otherwise compilation will fail** (Chinese + SSL + miniz exceeds the default 1.3MB app partition).

---

## 🔑 User Configuration

Open the `.ino` file and find the "User Configuration Section" to fill in your own account information:

```cpp
// QWeather (https://console.qweather.com/)
const char* QWEATHER_API_KEY  = "Fill in QWeather api-key";
const char* QWEATHER_API_HOST = "Fill in QWeather api-host";

// apihz (https://www.apihz.cn/)
const char* LUNAR_API_ID  = "Fill in apihz developer ID";
const char* LUNAR_API_KEY = "Fill in apihz developer API-key";
```

### QWeather Configuration

1. Register at https://console.qweather.com/
2. "Project Management" → Create project
3. "Credentials" → Generate API Key
4. "Settings" → View API Host

### apihz Configuration

1. Register at https://www.apihz.cn/
2. You can find the **Developer ID** and **API Key** in the personal center
3. Fill them into the code

---

## 🎮 Button Operations

| Button                    | Clock/Weather Page       | Menu Page        | WiFi Scan      | Password Input                   |
| ------------------------- | ------------------------ | ---------------- | -------------- | -------------------------------- |
| **Left**                  | Switch to clock page     | Return           | Return to menu | Backspace                        |
| **Right**                 | Switch to weather page   | Enter option     | Select SSID    | Add character                    |
| **Up**                    | Enter menu               | Move option up   | Move list up   | Character +1                     |
| **Down**                  | —                        | Move option down | Move list down | Character -1                     |
| **Long press Left/Right** | Turn hourly chime on/off | —                | —              | Long press to confirm connection |

---

## 🔄 Data Update Frequency

| Data                         | Frequency                             |
| ---------------------------- | ------------------------------------- |
| NTP time                     | Sync once on boot + every 5 minutes   |
| Weather (current + 7 days)   | Fetch once on boot + every 10 minutes |
| Lunar calendar + solar terms | Once per day (refresh on day change)  |

---

## 📂 Version Description

This project has three branches:

- **`main (this branch)`**: Both versions included (except JWT No-Dial Version)
- **`Dial Version`**
- **`No-Dial Version`**
- **`JWT No-Dial Version`**

To switch branches:

Use the branch dropdown in the upper left corner of the GitHub page.

Special note: The No-Dial Version uses API key to access QWeather, while the JWT No-Dial Version uses JWT for access, which improves security but is more complex to configure. You can choose according to your needs. For usage of the JWT No-Dial Version, please refer to the README under the JWT No-Dial Version branch. Also, since the JWT No-Dial Version requires more files, please download them from the JWT No-Dial Version branch if needed.

## ⚠️ Notes

- **Weather functionality is only supported within China** (relies on ip9.com.cn geolocation + QWeather domestic API)
- **Lunar calendar functionality depends on the apihz API**, with low request frequency (once per day), and the free quota is sufficient
- **Do not disclose your API Key**, otherwise others will use up your quota
- **Do not push WiFi passwords or API Keys in plaintext to GitHub**

---

## 🙏 Acknowledgments

- Weather data: [QWeather](https://www.qweather.com/)
- Lunar calendar API: [apihz](https://www.apihz.cn/)
- IP geolocation: [ip9.com.cn](http://ip9.com.cn/)
- gzip decompression: [miniz](https://github.com/richgel999/miniz) (richgel999)
- Chinese font rendering: [U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX) (olikraus)
- Time sync: [NTPClient](https://github.com/arduino-libraries/NTPClient) (Fabrice Weinberg)

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
- **apihz.cn Lunar Calendar API**: Subject to apihz Terms of Service

---

## ⭐ If you find this project useful, feel free to give it a Star

---
