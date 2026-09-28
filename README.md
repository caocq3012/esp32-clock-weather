# ESP32-OLED-Weather-Clock

ESP32 128×64 OLED I2C 智能桌面天气时钟。支持中文显示、农历、节气、和风天气 V1 接口自动拉取当前天气 + 7 天预报、按键配网、离线缓存、夜间熄屏节能、整点蜂鸣报时。

---

## 📌 项目简介

基于 ESP32 开发板的 OLED 桌面时钟，适配 SSD1306 128×64 I2C OLED 屏幕。

主要功能：

- **自动 NTP 网络校时**：支持开机立即同步 + 定时刷新，离线可继续运行
- **农历 + 节气**：通过接口盒子（apihz.cn）在线获取农历月日、节气信息
- **IP 自动定位**：通过 ip9.com.cn 获取经纬度和城市名
- **和风天气 V1 接口**：当前天气 + 7 天预报，含 gzip 手动解压（miniz）
- **WiFi 配网**：扫描列表可视化选网，密码字符输入，XOR 加密存储到 Preferences
- **四按键交互**：页面切换、菜单选择、WiFi 密码输入、长按确认
- **整点蜂鸣报时**：支持开关
- **夜间自动熄屏**：21:00~07:00 超时熄屏，白天常亮
- **离线缓存**：断网时读取历史天气缓存
- **手绘天气图标**：晴天/多云/雨/雷/雪/雾/霾/沙尘等
- **串口心跳日志**：方便调试

---

## 🧰 硬件清单

| 硬件 | 型号/参数 |
|------|-----------|
| 主控 | ESP32 开发板（任意型号，Flash ≥ 4MB） |
| 屏幕 | SSD1306，128×64，I2C，地址 `0x3C`（淘宝链接：https://item.taobao.com/item.htm?id=767205498137    ，规格选4针IIC OLED液晶屏） |
| 按键 ×4 | 轻触开关 |
| 蜂鸣器 | 有源或无源，接 GPIO 23 |

tips：蜂鸣器接上去一直响不知道为什么有没有大神帮看看，不知道的话最好先别接

### 引脚定义

| 外设 | GPIO |
|------|------|
| OLED SDA | GPIO 19 |
| OLED SCL | GPIO 18 |
| 蜂鸣器 BEEP | GPIO 23 |
| KEY_UP 上按键 | GPIO 27 |
| KEY_DOWN 下按键 | GPIO 26 |
| KEY_LEFT 左按键 | GPIO 25 |
| KEY_RIGHT 右按键 | GPIO 33 |

> ⚠️ OLED 供电推荐 3.3V，不要接 5V，避免烧屏幕。I2C 地址默认 `0x3C`，如果黑屏可尝试改成 `0x3D`。

---

## 📦 依赖库

**Arduino IDE → 库管理器安装以下库：**

| 库名 | 作者 | 用途 |
|------|------|------|
| `Adafruit GFX Library` | Adafruit | 图形绘制基础库 |
| `Adafruit SSD1306` | Adafruit | OLED 屏幕驱动 |
| `U8g2_for_Adafruit_GFX` | olikraus | OLED 中文字体（GB2312） |
| `NTPClient` | Fabrice Weinberg | NTP 时间同步 |
| `ArduinoJson` | Benoit Blanchon | JSON 解析（**需 6.x 版本**） |

**项目文件自带：**

- `miniz.c` / `miniz.h` —— gzip 解压（手动实现 tinfl 解压）

**放入项目目录（与 `.ino` 同级）：**

```
esp32_clock_and_weather/
├── sketch_sep25a.ino
├── miniz.c
└── miniz.h
```

---

## ⚙️ 编译配置

Arduino IDE → **工具 → Partition Scheme** → 选：

```
Huge APP (3MB No OTA/1MB SPIFFS)
```

**否则会编译失败**（中文 + SSL + miniz 超过了默认 1.3MB app 分区）。

---

## 🔑 用户配置

打开 `.ino` 文件，找到「用户配置区」，填入你自己的账号信息：

```cpp
// 和风天气（https://console.qweather.com/）
const char* QWEATHER_API_KEY  = "填写和风api-key";
const char* QWEATHER_API_HOST = "填写和风api-host";

// 接口盒子（https://www.apihz.cn/）
const char* LUNAR_API_ID  = "填写接口盒子开发者ID";
const char* LUNAR_API_KEY = "填写接口盒子开发者API-key";
```

### 和风天气配置

1. 注册 https://console.qweather.com/
2. 「项目管理」→ 创建项目
3. 「凭据」→ 生成 API Key
4. 「设置」→ 查看 API Host

### 接口盒子配置

1. 注册 https://www.apihz.cn/
2. 个人中心里能看到 **开发者 ID** 和 **API Key**
3. 填入代码

---

## 🎮 按键操作

| 按键 | 时钟/天气页 | 菜单页 | WiFi 扫描 | 密码输入 |
|------|------------|--------|-----------|---------|
| **左** | 切时钟页 | 返回 | 返回菜单 | 退格 |
| **右** | 切天气页 | 进入选项 | 选中 SSID | 添加字符 |
| **上** | 进菜单 | 选项上移 | 列表上移 | 字符 +1 |
| **下** | — | 选项下移 | 列表下移 | 字符 -1 |
| **长按左/右** | 开/关整点报时 | — | — | 长按确认连接 |

---

## 🔄 数据更新频率

| 数据 | 频率 |
|------|------|
| NTP 时间 | 开机同步一次 + 每 5 分钟 |
| 天气（当前 + 7 天） | 开机拉一次 + 每 10 分钟 |
| 农历 + 节气 | 每天一次（跨天刷新） |

---


## 📂 版本说明

本项目有三个分支：

- **`main`**（当前）：两个版本都有
- **`表盘版`**
- **`无表盘版`**

切换分支：

在 GitHub 页面左上角的分支下拉框切换。

## ⚠️ 注意事项

- **天气功能仅支持中国境内使用**（依赖 ip9.com.cn 定位 + 和风天气国内接口）
- **农历功能依赖接口盒子 API**，请求频率低（每天一次），免费额度足够
- **API Key 请勿公开**，否则会被别人用你的额度
- **不要将 WiFi 密码、API Key 明文 push 到 GitHub**

---

## 🙏 致谢

- 天气数据：[和风天气](https://www.qweather.com/)
- 农历 API：[接口盒子](https://www.apihz.cn/)
- IP 定位：[ip9.com.cn](http://ip9.com.cn/)
- gzip 解压：[miniz](https://github.com/richgel999/miniz)（richgel999）
- 中文字体渲染：[U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX)（olikraus）
- 时间同步：[NTPClient](https://github.com/arduino-libraries/NTPClient)（Fabrice Weinberg）

---

## 📄 License

本项目采用 [MIT License](LICENSE) 开源协议，Copyright (c) 2026 gaocq。


### 第三方组件

本项目使用了以下第三方开源组件，各自遵循其原始协议：

- **[miniz](https://github.com/richgel999/miniz)** —— MIT License
  Copyright 2013-2014 RAD Game Tools and Valve Software
  Copyright 2010-2014 Rich Geldreich and Tenacious Software LLC
  
---

## ⭐ 如果你觉得这个项目有用，欢迎点个 Star
---


