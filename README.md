# ESP32-OLED-Weather-Clock
ESP32 128*64 OLED I2C 智能桌面天气时钟，支持中文显示、农历二十四节气、和风天气V1接口自动拉取天气+7天预报，按键配网，离线缓存，夜间熄屏节能，整点蜂鸣报时。

## 📌 项目简介
这是一款基于ESP32开发板的OLED桌面时钟项目，适配SSD1306 128×64 I2C OLED屏幕。
- 自动NTP网络校时，支持离线继续运行
- 公历自动换算农历 + 二十四节气识别
- IP自动定位城市，对接和风天气V1接口，支持gzip解压接口返回数据
- 7天天气预报缓存，断网离线模式读取历史天气缓存
- WiFi扫描列表可视化配网，密码XOR加密存储到Preferences，明文不保存
- 四按键人机交互：多页面切换、WiFi密码字符选择输入、长按确认
- 整点蜂鸣器报时，支持开关报时功能
- 夜间自动熄屏省电逻辑（21点~7点超时熄屏）
- 内置WiFi信号图标、手绘简易天气图标绘制
- 串口心跳日志，方便调试

## ✨ 硬件引脚定义
| 外设 | GPIO引脚 |
| ---- | ---- |
| OLED SDA | GPIO19 |
| OLED SCL | GPIO18 |
| 蜂鸣器BEEP_PIN | GPIO23 |
| KEY_UP 上按键 | GPIO27 |
| KEY_DOWN 下按键 | GPIO26 |
| KEY_LEFT 左按键 | GPIO25 |
| KEY_RIGHT 右按键 | GPIO33 |

> OLED供电推荐3.3V，不要直接接5V，避免烧屏幕
> I2C OLED地址默认 `0x3C`，如果黑屏可尝试修改为0x3D

## 📦 依赖库清单
Arduino库管理器安装下面全部库：
1. `Adafruit GFX Library`
2. `Adafruit SSD1306`
3. `U8g2_for_Adafruit_GFX` （用于OLED中文GB2312字体渲染）
4. `NTPClient`
5. `ArduinoJson`
6. `miniz` （gzip解压，适配和风天气压缩返回数据）

> ⚠️ 注意：代码内置miniz tinfl解压函数，用来解析和风V1接口gzip压缩报文，无需额外解压组件

## ⚙️ 用户配置
打开ino代码，找到【用户配置区】修改：
```cpp
const char* QWEATHER_API_KEY = "填写自己的和风apikey";
const char* QWEATHER_API_HOST = "填写自己的和风apihost";
```
## ⚠️ 注意事项
**该项目目前仅支持中国境内使用，谢谢谅解**

**This project currently only supports usage within mainland China. Thank you for your understanding**
