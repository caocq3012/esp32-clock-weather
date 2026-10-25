# esp32_clock_and_weather JWT No-Dial Version Special Notes

An ESP32-based desktop smart clock that supports OLED display, real-time weather, 7-day forecast, lunar calendar, NTP time synchronization, WiFi provisioning, hourly chime, and other functions.

## Features

- ⏰ **Clock display**: Large-font time + date + weekday + lunar calendar
- 🌤 **Real-time weather**: Automatic IP geolocation, displays city, weather, temperature, and humidity
- 📅 **7-day forecast**: QWeather daily API, icons + dates
- 🌙 **Lunar calendar display**: Obtains lunar dates via the apihz.cn API
- 📡 **NTP time synchronization**: Automatically syncs every 5 minutes
- 🔌 **WiFi provisioning**: Supports scanning / manual entry of SSID and password
- 🔔 **Hourly chime**: Buzzer rings on the hour (can be toggled)
- 🌙 **Night screen-off**: Automatically turns off the screen after 10 seconds from 21:00 to 07:00 to save power; press the down key to turn the screen on for 10 seconds; stays on during the day
- 💾 **Offline cache**: Weather data is cached locally and can still be displayed when offline
- 🔐**High security**: Uses JWT authentication, which is more secure than API key

## Hardware Requirements

| Component | Model / Parameters                        |
| --------- | ----------------------------------------- |
| MCU       | ESP32 (ESP32-DevKitC or compatible board) |
| Display   | SSD1306 OLED 128×64 (I2C, address 0x3C)   |
| Buzzer    | Passive/active buzzer                     |
| Buttons   | 4 tactile buttons (up/down/left/right)    |

### Pin Connections

| Function     | GPIO |
| ------------ | ---- |
| OLED SDA     | 19   |
| OLED SCL     | 18   |
| Buzzer       | 23   |
| Button UP    | 27   |
| Button DOWN  | 26   |
| Button LEFT  | 25   |
| Button RIGHT | 33   |

## Dependencies

Install in the Arduino IDE Library Manager:

- `Adafruit GFX Library`
- `Adafruit SSD1306`
- `ArduinoJson` (7.x)
- `NTPClient`
- `U8g2_for_Adafruit_GFX`
- `WiFi` (built into ESP32)
- `HTTPClient` (built into ESP32)
- `WiFiClientSecure` (built into ESP32)
- `Preferences` (built into ESP32)

**Local files** (in the same directory as the source code; can be packaged and downloaded directly):

- `miniz.h` / `miniz.c` — used for gzip decompression
- `edsign.h` / `edsign.c`, etc. — Ed25519 signatures

## API Application

### QWeather API

1. Register at the [QWeather Console](https://console.qweather.com/)
2. Create a project and obtain:
   - `QWEATHER_USER_ID` (developer ID)
   - `QWEATHER_API_HOST` (API Host, in the form `xxx.re.qweatherapi.com`)
   - Obtain them in the top-right avatar -- Settings

### Generate an Ed25519 Key Pair

QWeather uses the Ed25519 algorithm for JWT signing, so a key pair must be generated.

**Method 1: Use OpenSSL (recommended)**

```bash
openssl genpkey -algorithm ED25519 -out ed25519-private.pem && openssl pkey -pubout -in ed25519-private.pem > ed25519-public.pem
```

Two files are generated:

- `ed25519-private.pem` — private key (keep it yourself; do not disclose it)
- `ed25519-public.pem` — public key (upload to the QWeather console)

**Method 2: Use the official QWeather JWT tool**

Visit [https://jwt.qweather.com](https://jwt.qweather.com). The page automatically generates a key pair, and you can directly copy the private key and public key.

**Method 3: Use the official QWeather sample code**

The official QWeather documentation provides JWT generation sample code in Java, Python, Node.js, and other languages. You can refer to the cryptography library for the corresponding language to generate an Ed25519 key pair.

### Upload the Public Key and Obtain Credentials

1. Log in to the QWeather console and go to **Project Management**
2. Select a project (create one if none exists) and click **"Add Credential"**
3. Enter any credential name
4. For the authentication method, select **"JSON Web Token"**
5. Copy the entire contents of `ed25519-public.pem` (including `-----BEGIN PUBLIC KEY-----` and `-----END PUBLIC KEY-----`) and paste it into the public key input box
6. After saving, record the **Credential ID** and **Project ID**; they will be **useful later**

### Convert the Private Key to a Hexadecimal Array

#### Method 1: Python Script (recommended)

**Prerequisite**: Install the `cryptography` library

```bash
pip install cryptography
```

**Script**:

```python
from cryptography.hazmat.primitives import serialization

with open("ed25519-private.pem", "rb") as f:
    key = serialization.load_pem_private_key(f.read(), password=None)

raw = key.private_bytes(
    encoding=serialization.Encoding.Raw,
    format=serialization.PrivateFormat.Raw,
    encryption_algorithm=serialization.NoEncryption()
)

print("static const uint8_t ED25519_PRIVATE_KEY[32] = {")
for i in range(0, 32, 8):
    line = ", ".join(f"0x{b:02x}" for b in raw[i:i+8])
    comma = "," if i + 8 < 32 else ""
    print(f"  {line}{comma}")
print("};")
```

**Usage**:

```bash
python convert.py
```

**Example output**:

```
static const uint8_t ED25519_PRIVATE_KEY[32] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
```

**Copy and paste it directly into the code.**

---

## Method 2: OpenSSL One-Liner

```bash
openssl pkey -in ed25519-private.pem -outform DER | tail -c 32 | xxd -p -c 8 | sed 's/../0x&, /g'
```

**Example output**

```
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
...
```

**Manually add `{ }` and line breaks yourself.**

---

## Method 3: Use the Official QWeather JWT Tool

Visit [https://jwt.qweather.com](https://jwt.qweather.com)

- The page **directly displays the private key in 32-byte hexadecimal format** (e.g., `c1b5c77b...`)
- Add `0x` and `,` every two characters to get the array format

**Copying by hand is too tedious, so use Method 1 or 2.**

### Interface Box API

1. Register at [Interface Box](https://www.apihz.cn/)
2. Obtain:
   - `LUNAR_API_ID` (developer ID)
   - `LUNAR_API_KEY` (API Key)

## Quick Start

1. **Clone / download** this repository into the Arduino sketch directory.

2. Modify the **user configuration section**:

   ```cpp
   // QWeather configuration
   const char* QWEATHER_USER_ID       = "Fill in the QWeather developer ID";
   const char* QWEATHER_PROJECT_ID    = "Fill in the QWeather project ID";
   const char* QWEATHER_CREDENTIAL_ID = "Fill in the QWeather credential ID";
   const char* QWEATHER_API_HOST      = "Fill in the QWeather API host";
   
   static const uint8_t ED25519_PRIVATE_KEY[32] = {
     // Fill in the generated private key (32-byte hexadecimal array)
   };
   
   // Interface Box configuration
   const char* LUNAR_API_ID  = "Fill in the Interface Box developer ID";
   const char* LUNAR_API_KEY = "Fill in the Interface Box API key";
   ```

3. **Compile and flash**. On first startup, it will connect to the saved WiFi; if none is saved, it will enter the provisioning process.

4. **Button operations**:

   - Clock page: Left/Right switches pages, Up enters the menu
   - Menu page: Up/Down selects, Right confirms, Left returns
   - Long press Left/Right: Toggle the hourly chime

5. **⚠️Note** When compiling, you need to select **Huge APP (3MB No OTA/1MB SPIFFS)** in **Tools -- Partition Scheme**, otherwise it cannot compile and upload properly!!!

## Architecture Overview

```
setup()
  ├─ OLED / u8g2 initialization
  ├─ prefs initialization
  ├─ tinfl decompressor preallocation (10KB on the heap)
  ├─ WiFi connection
  ├─ NTP synchronization
  └─ Startup animation

loop()
  ├─ Button scanning
  ├─ WiFi reconnection
  ├─ NTP scheduled synchronization (5 minutes)
  ├─ Weather update (10 minutes)
  │   ├─ ip9 geolocation
  │   ├─ QWeather current (JWT + Ed25519 authentication)
  │   ├─ gzip decompression (tinfl reuse)
  │   ├─ QWeather daily 7-day forecast
  │   └─ gzip decompression
  └─ Page drawing
```

## Key Technical Points

### 1. JWT Authentication (QWeather API)

QWeather requires API requests to carry an **Ed25519-signed JWT**. This project's implementation:

- Uses the Ed25519 private key to sign the JWT header + payload
- Generates a complete JWT encoded in base64url
- Passes it to the API via the `Authorization: Bearer <jwt>` header

### 2. gzip Streaming Decompression

The QWeather API returns gzip-compressed data. To decompress it under the ESP32's limited memory:

- Uses `tinfl_decompressor_alloc()` to allocate the decompressor **on the heap** (about 10KB)
- **Globally reuses the same decompressor** to avoid repeated malloc/free causing heap fragmentation
- Adds the `TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF` flag to output to a large contiguous buffer

### 3. HTTPS Connection Reuse

The two HTTPS requests (current + daily) reuse the same `WiFiClientSecure` to avoid `code=-1` errors caused by repeated allocation of TLS contexts.

## Directory Structure

```
sketch_sep25a/
├── sketch_sep25a.ino   # Main program
├── miniz.h             # miniz header file
├── miniz.c             # miniz implementation
├── edsign.h            # Ed25519 signing interface
├── edsign.c
├── ed25519.c
├── c25519.c
├── f25519.c
├── fprime.c
├── morph25519.c
└── sha512.c
```

## FAQ

**Q: Compilation reports `undefined reference to tinfl_decompress_mem_to_mem`?**

A: Make sure `miniz.h` / `miniz.c` are in the sketch directory and are not overridden by other miniz copies in the Arduino Library Manager.

**Q: Weather shows "No data"?**

A: Check whether WiFi is connected, whether the QWeather API credentials are correct, and whether the private key matches the public key in the console.

**Q: The forecast API reports `code=-1`?**

A: Insufficient heap memory causes the TLS handshake to fail. Make sure `g_secureClient` is globally reused and that there is a `delay(100)` between the two HTTPS requests.

**Q: JWT generation fails?**

A: Check whether NTP synchronization succeeded (JWT depends on the system time) and whether `ED25519_PRIVATE_KEY` is filled in correctly.

**Q: The lunar calendar is not displayed?**

A: Check whether the Interface Box `LUNAR_API_ID` and `LUNAR_API_KEY` are configured correctly, and whether the API quota has been used up.

## Acknowledgments

- **Ed25519 signatures**: The Ed25519 signature implementation used in this project comes from
  [iot-tor/esp32-ed25519](https://github.com/iot-tor/esp32-ed25519),
  and is used to generate JWT signatures for the QWeather API. Thanks to the original author for the open-source contribution.
- **miniz**: gzip/deflate compression library, by Rich Geldreich.
- **QWeather**: Provides real-time weather and 7-day forecast APIs.
- **apihz.cn**: Provides the lunar calendar API.

## Third-Party Component Licenses

- **Ed25519 signatures**: From [iot-tor/esp32-ed25519](https://github.com/iot-tor/esp32-ed25519),
  which **does not declare a license**. This project uses its source code only by reference,
  does not change the licensing status of the original project, and users must assess compliance themselves.
- **miniz**: MIT / Public Domain
- **Adafruit GFX / SSD1306**: BSD
- **ArduinoJson**: MIT
- **NTPClient**: MIT
- **U8g2_for_Adafruit_GFX**: MIT
- **QWeather API**: Subject to the QWeather Terms of Service
- **apihz.cn lunar calendar API**: Subject to the apihz Terms of Service

## Notes

- API credentials in this project (QWeather private key, API Key, etc.) **must not be committed to public repositories**.
  It is recommended to extract the configuration into a separate `secrets.h` file and add it to `.gitignore`.
- QWeather's free quota is limited, so it is recommended to set a reasonable update interval (default 10 minutes).
- On first startup, if WiFi is not configured, it will enter provisioning mode, where you enter the SSID and password via buttons.

## License

The code in this project is released under the MIT License (see the [LICENSE](LICENSE) file for details).
