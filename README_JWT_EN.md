# esp32_clock_and_weather JWT No-Dial Version Special Notes

---

[简体中文](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT.md)

[English](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT_EN.md)

[日本語](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT_JP.md)

---

An ESP32-based desktop smart clock supporting OLED display, real-time weather, 7-day forecast, lunar calendar, NTP time synchronization, WiFi provisioning, hourly chime, and more.

## Features

- ⏰ **Clock Display**: Large-font time + date + day of week + lunar calendar
- 🌤 **Real-time Weather**: Automatic IP-based location, displaying city, weather, temperature, and humidity
- 📅 **7-Day Forecast**: QWeather daily API, icons + dates
- 🌙 **Lunar Calendar Display**: Lunar date retrieved via apihz.cn API
- 📡 **NTP Time Sync**: Automatic synchronization every 5 minutes
- 🔌 **WiFi Provisioning**: Supports scanning / manual entry of SSID and password
- 🔔 **Hourly Chime**: Buzzer rings on the hour (can be toggled)
- 🌙 **Night Screen Off**: Auto screen-off 10 seconds after 21:00~07:00 to save power; press the down key to light up the screen for 10 seconds; stays on during the day
- 💾 **Offline Cache**: Weather data cached locally, still displayable when offline
- 🔐 **High Security**: Uses JWT authentication, more secure than API key

## Hardware Requirements

| Component | Model / Parameters                        |
| --------- | ----------------------------------------- |
| MCU       | ESP32 (ESP32-DevKitC or compatible board) |
| Display   | SSD1306 OLED 128×64 (I2C, address 0x3C)   |
| Buzzer    | Passive/Active buzzer                     |
| Buttons   | 4 tactile buttons (Up/Down/Left/Right)    |

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
- `WiFi` (bundled with ESP32)
- `HTTPClient` (bundled with ESP32)
- `WiFiClientSecure` (bundled with ESP32)
- `Preferences` (bundled with ESP32)

**Local files** (at the same level as the source code, can be downloaded directly as a package):

- `miniz.h` / `miniz.c` — for gzip decompression
- `ed25519.h` / `fe.c` / `ge.c` / `sc.c` / `sha512.c` / `keypair.c` / `sign.c` / `verify.c` etc. — Ed25519 signatures (orlp/ed25519, zlib license)

## API Application

### QWeather API

1. Register at [QWeather Console](https://console.qweather.com/)
2. Create a project and obtain:
   - `QWEATHER_USER_ID` (Developer ID)
   - `QWEATHER_API_HOST` (API Host, in the form `xxx.re.qweatherapi.com`)
   - Obtain from the top-right avatar -- Settings

### Generate Ed25519 Key Pair

QWeather uses the Ed25519 algorithm for JWT signing, so a key pair needs to be generated.

**Method 1: Using OpenSSL (Recommended)**

```bash
openssl genpkey -algorithm ED25519 -out ed25519-private.pem && openssl pkey -pubout -in ed25519-private.pem > ed25519-public.pem
```

Two files are generated:

- `ed25519-private.pem` — Private key (keep it safe, do not leak)
- `ed25519-public.pem` — Public key (upload to QWeather Console)

**Method 2: Using the Official QWeather JWT Tool**

Visit [https://jwt.qweather.com](https://jwt.qweather.com); the page will automatically generate a key pair, and you can directly copy the private and public keys.

**Method 3: Using the Official QWeather Sample Code**

The official QWeather documentation provides JWT generation sample code in Java, Python, Node.js, and other languages. You can refer to the cryptography library of the corresponding language to generate an Ed25519 key pair.

### Upload Public Key and Obtain Credentials

1. Log in to the QWeather Console and go to **Project Management**
2. Select a project (create one if none exists) and click **"Add Credential"**
3. Enter any credential name
4. Select **"JSON Web Token"** as the authentication method
5. Copy the entire contents of `ed25519-public.pem` (including `-----BEGIN PUBLIC KEY-----` and `-----END PUBLIC KEY-----`) and paste it into the public key input box
6. After saving, record the **Credential ID** and **Project ID** — these will be **useful later**

### Convert Private Key to Hexadecimal Array

#### Method 1: Python Script (Recommended)

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

**Example Output**:

```
static const uint8_t ED25519_PRIVATE_KEY[32] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
```

**Copy and paste directly into the code.**

---

## Method 2: OpenSSL One-Liner

```bash
openssl pkey -in ed25519-private.pem -outform DER | tail -c 32 | xxd -p -c 8 | sed 's/../0x&, /g'
```

**Sample Output**

```
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
...
```

**Manually add `{ }` and line breaks yourself.**

---

## Method 3: Using the Official QWeather JWT Tool

Visit [https://jwt.qweather.com](https://jwt.qweather.com)

- The page **directly displays the private key in 32-byte hexadecimal format** (e.g., `c1b5c77b...`)
- Add `0x` and `,` before each pair of characters to form the array format

**Copying by hand is too tedious — use Method 1 or 2 instead.**

### Jiekou Hezi API

1. Register at [Jiekou Hezi](https://www.apihz.cn/)
2. Obtain:
   - `LUNAR_API_ID` (Developer ID)
   - `LUNAR_API_KEY` (API Key)

## Quick Start

1. **Clone / Download** this repository to your Arduino sketch directory.

2. Modify the **User Configuration Section**:

   ```cpp
   // QWeather configuration
   const char* QWEATHER_USER_ID       = "Enter QWeather developer ID";
   const char* QWEATHER_PROJECT_ID    = "Enter QWeather project ID";
   const char* QWEATHER_CREDENTIAL_ID = "Enter QWeather credential ID";
   const char* QWEATHER_API_HOST      = "Enter QWeather API host";
   
   static const uint8_t ED25519_PRIVATE_KEY[32] = {
     // Enter the generated private key (32-byte hexadecimal array)
   };
   
   // Jiekou Hezi configuration
   const char* LUNAR_API_ID  = "Enter Jiekou Hezi developer ID";
   const char* LUNAR_API_KEY = "Enter Jiekou Hezi API key";
   ```

3. **Compile and flash**. On first startup, it will connect to the saved WiFi; if none is saved, it enters the provisioning flow.

4. **Button Operations**:

   - Clock page: Left/Right to switch pages, Up to enter the menu
   - Menu page: Up/Down to select, Right to confirm, Left to return
   - Long press Left/Right: Toggle hourly chime

5. **⚠Note** When compiling, you need to select **Huge APP (3MB No OTA/1MB SPIFFS)** in **Tools -- Partition Scheme**, otherwise it cannot compile and upload properly!!

## Architecture Overview

```
setup()
  ├─ OLED / u8g2 initialization
  ├─ prefs initialization
  ├─ tinfl decompressor pre-allocation (10KB on heap)
  ├─ WiFi connection
  ├─ NTP sync
  └─ Boot animation

loop()
  ├─ Button scanning
  ├─ WiFi reconnection
  ├─ NTP periodic sync (5 minutes)
  ├─ Weather update (10 minutes)
  │   ├─ ip9 location
  │   ├─ QWeather current (JWT + Ed25519 authentication)
  │   ├─ gzip decompression (tinfl reused)
  │   ├─ QWeather daily 7-day forecast
  │   └─ gzip decompression
  └─ Page rendering
```

## Key Technical Points

### 1. JWT Authentication (QWeather API)

QWeather requires API requests to carry an **Ed25519-signed JWT**. This project's implementation:

- Uses a 32-byte seed to generate the public key (32B) and private key (64B) via `ed25519_create_keypair()`
- Uses `ed25519_sign()` to sign `header.payload`
- Base64url encodes and assembles the complete JWT, passing it to the API via the `Authorization: Bearer <jwt>` header

### 2. gzip Streaming Decompression

The QWeather API returns gzip-compressed data. To decompress within the limited memory of the ESP32:

- Uses `tinfl_decompressor_alloc()` to allocate the decompressor **on the heap** (about 10KB)
- **Globally reuses the same decompressor** to avoid repeated malloc/free causing heap fragmentation
- Adds the `TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF` flag to output to a contiguous large buffer

### 3. HTTPS Connection Reuse

Two HTTPS requests (current + daily) reuse the same `WiFiClientSecure` to avoid the `code=-1` error caused by repeated TLS context allocation.

## Directory Structure

```
sketch_sep25a/
├── sketch_sep25a.ino   # Main program
├── miniz.h             # miniz header
├── miniz.c             # miniz implementation
├── ed25519.h           # Ed25519 interface (orlp/ed25519)
├── fixedint.h          # Fixed-length integer types
├── fe.c / fe.h         # Field arithmetic
├── ge.c / ge.h         # Group arithmetic
├── sc.c / sc.h         # Scalar arithmetic
├── sha512.c / sha512.h # SHA-512
├── keypair.c           # Key generation
├── sign.c              # Signing
├── verify.c            # Verification
└── precomp_data.h      # Precomputed tables
```

## FAQ

**Q: Compilation error `undefined reference to tinfl_decompress_mem_to_mem`?**

A: Make sure `miniz.h` / `miniz.c` are in the sketch directory and are not overridden by other miniz libraries in the Arduino Library Manager.

**Q: Compilation error `'edsign_sec_to_pub' was not declared in this scope`?**

A: This project has switched to `orlp/ed25519` and no longer uses the `edsign` series of functions. Please confirm that `ed25519.h` and `fe.c` / `ge.c` / `sc.c` / `sha512.c` / `keypair.c` / `sign.c` / `verify.c` are all in the sketch directory, and that the code calls `ed25519_create_keypair()` and `ed25519_sign()`.

**Q: Weather shows "No Data"?**

A: Check whether WiFi is connected, whether the QWeather API credentials are correct, and whether the private key matches the public key in the console.

**Q: Forecast API returns `code=-1`?**

A: Insufficient heap memory causes TLS handshake failure. Make sure `g_secureClient` is globally reused, and there is a `delay(100)` between the two HTTPS requests.

**Q: JWT generation fails?**

A: Check whether NTP synchronization is successful (JWT depends on system time), and whether `ED25519_PRIVATE_KEY` is correctly filled in.

**Q: Lunar calendar not displayed?**

A: Check whether Jiekou Hezi's `LUNAR_API_ID` and `LUNAR_API_KEY` are correctly configured, and whether the API quota has been exhausted.

## Acknowledgments

- **Ed25519 Signatures**: This project uses [orlp/ed25519](https://github.com/orlp/ed25519)
  (zlib license, Copyright (c) 2015 Orson Peters) to implement JWT signing. Thanks to the original author for the open-source contribution.
- **miniz**: gzip/deflate compression library, by Rich Geldreich.
- **QWeather**: Provides real-time weather and 7-day forecast APIs.
- **apihz.cn**: Provides the lunar calendar API.

## Third-Party Component Licenses

- **Ed25519 Signatures**: [orlp/ed25519](https://github.com/orlp/ed25519), zlib license.
  Its copyright notice and full license text must be retained when used; modified source code must be marked as "modified".
- **miniz**: MIT / Public Domain
- **Adafruit GFX / SSD1306**: BSD
- **ArduinoJson**: MIT
- **NTPClient**: MIT
- **U8g2_for_Adafruit_GFX**: MIT
- **QWeather API**: Subject to QWeather's Terms of Service
- **apihz.cn Lunar Calendar API**: Subject to apihz's Terms of Service

The complete license statements for third-party dependencies can be found in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Notes

- The API credentials in this project (QWeather private key, API Key, etc.) **must not be committed to public repositories**.
  It is recommended to extract the configuration into a separate `secrets.h` file and add it to `.gitignore`.
- QWeather's free quota is limited; it is recommended to set a reasonable update interval (default 10 minutes).
- On first startup, if WiFi is not configured, it will enter provisioning mode, where you enter the SSID and password via buttons.

## License

The code in this project is released under the MIT License (see the [LICENSE](LICENSE) file for details).
