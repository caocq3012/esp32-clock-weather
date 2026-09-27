# esp32_clock_and_weather
esp32_clock_and_weather
欢迎使用esp32_clock_and_weather！
本程序采用Arduino  IDE软件针对esp32 dev module开发
本程序需要额外安装的环境是：
# 项目依赖总结

## 一、Arduino IDE 环境

| 项目 | 版本要求 |
|------|----------|
| **Arduino IDE** | 2.0+ 或 1.8.x |
| **开发板管理器** | esp32 by Espressif Systems（推荐 3.x） |
| **Partition Scheme** | **Huge APP (3MB No OTA/1MB SPIFFS)** ← 必选 |

---

## 二、库依赖（Arduino 库管理器安装）

| 库名 | 作者 | 用途 |
|------|------|------|
| **Adafruit SSD1306** | Adafruit | OLED 屏幕驱动 |
| **Adafruit GFX Library** | Adafruit | 图形绘制基础库 |
| **ArduinoJson** | Benoit Blanchon | JSON 解析（**6.x 版本，不是 7.x**） |
| **U8g2_for_Adafruit_GFX** | olikraus | 中文字体渲染（叠加在 Adafruit GFX 上） |
| **NTPClient** | Fabrice Weinberg | NTP 时间同步 |
| **Preferences** | ESP32 自带 | Flash 参数存储 |

---

## 三、项目文件依赖（放到 `.ino` 同目录）

| 文件 | 来源 | 用途 |
|------|------|------|
| **miniz.c** | https://github.com/richgel999/miniz/releases | gzip 解压 |
| **miniz.h** | 同上 | miniz 头文件 |

**目录结构：**
```
sketch_sep25a/
├── sketch_sep25a.ino
├── miniz.c
└── miniz.h
```

---

## 四、ESP32 core 自带的库（不用装）

| 头文件 | 说明 |
|--------|------|
| `<WiFi.h>` | WiFi 连接 |
| `<WiFiClientSecure.h>` | HTTPS 客户端 |
| `<HTTPClient.h>` | HTTP 请求 |
| `<Wire.h>` | I2C 通信（OLED 用） |
| `<WiFiUdp.h>` | UDP（NTPClient 用） |
| `<time.h>` | 时间函数 |
| `<stdint.h>` / `<string.h>` | 标准库 |

---

## 五、中文字体依赖

**`U8g2_for_Adafruit_GFX` 库自带中文字体，代码里用的：**

```cpp
u8g2.setFont(u8g2_font_wqy12_t_gb2312);
```

**这是 GB2312 一级字库（3755 个常用汉字），约 200KB Flash。**

**如果换其他字体：**
- `u8g2_font_wqy12_t_chinese1` — 更大字库
- `u8g2_font_wqy16_t_gb2312` — 16px 字号（更清晰，占更多 Flash）

---

## 六、外部服务依赖

| 服务 | 地址 | 用途 | 是否需要注册 |
|------|------|------|-------------|
| **ip9.com.cn** | `http://ip9.com.cn/get` | IP 定位（获取经纬度/城市） | ❌ 免费，无需注册 |
| **和风天气** | `https://ke7fc4nn45.re.qweatherapi.com` | 天气数据 | ✅ 需注册，免费额度 1000 次/天 |
| **阿里云 NTP** | `ntp.aliyun.com` / `ntp1.aliyun.com` | 时间同步 | ❌ 免费 |
| **国家授时中心** | `ntp.ntsc.ac.cn` | 时间同步备用 | ❌ 免费 |

---

## 七、和风天气账号配置

**登录 https://console.qweather.com/ 需要拿到：**

| 参数 | 说明 | 代码里的变量 |
|------|------|-------------|
| **API Key** | 在"项目管理 → 凭据"里生成 | `QWEATHER_API_KEY` |
| **API Host** | 在"设置 → API Host"里查看 | `QWEATHER_API_HOST` |

**代码里替换：**
```cpp
const char* QWEATHER_API_KEY = "你的API_KEY";
const char* QWEATHER_API_HOST = "你的API_HOST";
```

---

## 八、硬件依赖

| 硬件 | 型号 | 引脚 |
|------|------|------|
| **主控** | ESP32（任意型号，4MB Flash 以上） | — |
| **OLED** | SSD1306，128×64，I2C | SDA=19, SCL=18 |
| **按键 ×4** | 轻触开关 | UP=27, DOWN=26, LEFT=25, RIGHT=33 |
| **蜂鸣器** | 有源/无源 | BEEP=23 |

---

## 九、一句话总结

**核心依赖就 4 个：**
1. **ESP32 core 3.x + Huge APP 分区**
2. **miniz.c + miniz.h**（放项目目录）
3. **ArduinoJson 6.x + U8g2_for_Adafruit_GFX + Adafruit SSD1306 + NTPClient**（库管理器装）
4. **和风天气 API Key**（自己注册）

**其他全是 ESP32 自带或免费服务，不用额外装。**

# 谢谢支持！
