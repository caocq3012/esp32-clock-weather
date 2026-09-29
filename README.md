# esp32_clock_and_weather JWT无表盘版特殊说明

基于 ESP32 的桌面智能时钟，支持 OLED 显示、实时天气、7 天预报、农历、NTP 校时、WiFi 配网、整点报时等功能。

## 功能特性

- ⏰ **时钟显示**：大字体时间 + 日期 + 星期 + 农历
- 🌤 **实时天气**：自动 IP 定位，显示城市、天气、温度、湿度
- 📅 **7 天预报**：和风天气 daily 接口，图标 + 日期
- 🌙 **农历显示**：通过 apihz.cn API 获取农历日期
- 📡 **NTP 校时**：每 5 分钟自动同步
- 🔌 **WiFi 配网**：支持扫描 / 手动输入 SSID 和密码
- 🔔 **整点报时**：蜂鸣器整点响铃（可开关）
- 🌙 **夜间息屏**：21:00~07:00 10秒后自动熄屏省电，按下down键亮屏10秒
- 💾 **离线缓存**：天气数据本地缓存，断网时仍可显示
- 🔐**安全性高**：使用JWT认证，比APIkey安全性强

## 硬件需求

| 组件   | 型号 / 参数                           |
| ------ | ------------------------------------- |
| 主控   | ESP32（ESP32-DevKitC 或兼容板）       |
| 显示屏 | SSD1306 OLED 128×64（I2C，地址 0x3C） |
| 蜂鸣器 | 无源/有源蜂鸣器                       |
| 按键   | 4 个轻触按键（上/下/左/右）           |

### 引脚连接

| 功能       | GPIO |
| ---------- | ---- |
| OLED SDA   | 19   |
| OLED SCL   | 18   |
| 蜂鸣器     | 23   |
| 按键 UP    | 27   |
| 按键 DOWN  | 26   |
| 按键 LEFT  | 25   |
| 按键 RIGHT | 33   |

## 依赖库

在 Arduino IDE 的库管理器中安装：

- `Adafruit GFX Library`
- `Adafruit SSD1306`
- `ArduinoJson`（7.x）
- `NTPClient`
- `U8g2_for_Adafruit_GFX`
- `WiFi`（ESP32 自带）
- `HTTPClient`（ESP32 自带）
- `WiFiClientSecure`（ESP32 自带）
- `Preferences`（ESP32 自带）

**本地文件**（与源代码同级，可以直接打包下载）：

- `miniz.h` / `miniz.c` — 用于 gzip 解压
- `edsign.h` / `edsign.c` 等 — Ed25519 签名

## API 申请

### 和风天气 API

1. 注册 [和风天气控制台](https://console.qweather.com/)
2. 创建项目，获取：
   - `QWEATHER_USER_ID`（开发者 ID）
   - `QWEATHER_API_HOST`（API Host，形如 `xxx.re.qweatherapi.com`）
   - 在右上角头像--设置中获取

### 生成 Ed25519 密钥对

和风天气使用 Ed25519 算法进行 JWT 签名，需要生成密钥对。

**方法一：使用 OpenSSL（推荐）**

```bash
openssl genpkey -algorithm ED25519 -out ed25519-private.pem && openssl pkey -pubout -in ed25519-private.pem > ed25519-public.pem
```

生成两个文件：

- `ed25519-private.pem` — 私钥（自行保管，不要泄露）
- `ed25519-public.pem` — 公钥（上传到和风控制台）

**方法二：使用和风天气官方 JWT 工具**

访问 [https://jwt.qweather.com](https://jwt.qweather.com)，页面会自动生成密钥对，可直接复制私钥和公钥。

**方法三：使用和风天气官方示例代码**

和风官方文档提供了 Java、Python、Node.js 等语言的 JWT 生成示例代码，可参考对应语言的密码学库生成 Ed25519 密钥对。

### 上传公钥并获取凭据

1. 登录和风天气控制台，进入 **项目管理**
2. 选择项目（如果没有新建一个），点击 **"添加凭据"**
3. 凭据名称任意填写
4. 身份认证方式选择 **"JSON Web Token"**
5. 复制 `ed25519-public.pem` 的全部内容（含 `-----BEGIN PUBLIC KEY-----` 和 `-----END PUBLIC KEY-----`），粘贴到公钥输入框
6. 保存后记录 **凭据 ID** 和 **项目 ID**之后**有用**

### 将私钥转换为十六进制数组

#### 方法一：Python 脚本（推荐）

**前提**：安装 `cryptography` 库

```bash
pip install cryptography
```

**脚本**：

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

**用法**：

```bash
python convert.py
```

**输出示例**：

```
static const uint8_t ED25519_PRIVATE_KEY[32] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
```

**直接复制粘贴到代码里。**

---

## 方法二：OpenSSL 一行命令

```bash
openssl pkey -in ed25519-private.pem -outform DER | tail -c 32 | xxd -p -c 8 | sed 's/../0x&, /g'
```

**输出样例**

```
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
...
```

**自己手动加 `{ }` 和换行。**

---

## 方法三：用和风官方 JWT 工具

访问 [https://jwt.qweather.com](https://jwt.qweather.com)

- 页面**直接显示 32 字节十六进制格式**的私钥（如 `c1b5c77b...`）
- 每两个字符加 `0x` 和 `,` 就是数组格式

**手抄太累，还是用方法一或二。**

### 接口盒子 API

1. 注册 [接口盒子](https://www.apihz.cn/)
2. 获取：
   - `LUNAR_API_ID`（开发者 ID）
   - `LUNAR_API_KEY`（API Key）

## 快速开始

1. **克隆 / 下载**本仓库到 Arduino sketch 目录。

2. 修改 **用户配置区**：

   ```cpp
   // 和风天气配置
   const char* QWEATHER_USER_ID       = "填写和风开发者ID";
   const char* QWEATHER_PROJECT_ID    = "填写和风项目ID";
   const char* QWEATHER_CREDENTIAL_ID = "填写和风凭据ID";
   const char* QWEATHER_API_HOST      = "填写和风APIhost";
   
   static const uint8_t ED25519_PRIVATE_KEY[32] = {
     // 填写生成的私钥（32 字节十六进制数组）
   };
   
   // 接口盒子配置
   const char* LUNAR_API_ID  = "填写接口盒子开发者ID";
   const char* LUNAR_API_KEY = "填写接口盒子APIkey";
   ```

3. **编译烧录**，首次启动会连接已保存的 WiFi，未保存时进入配网流程。

4. **按键操作**：

   - 时钟页：左/右切换页面，上进入菜单
   - 菜单页：上/下选择，右确认，左返回
   - 长按左/右：开关整点报时

## 架构概览

```
setup()
  ├─ OLED / u8g2 初始化
  ├─ prefs 初始化
  ├─ tinfl 解压器预分配（堆上 10KB）
  ├─ WiFi 连接
  ├─ NTP 同步
  └─ 启动动画

loop()
  ├─ 按键扫描
  ├─ WiFi 重连
  ├─ NTP 定时同步（5 分钟）
  ├─ 天气更新（10 分钟）
  │   ├─ ip9 定位
  │   ├─ 和风 current（JWT + Ed25519 鉴权）
  │   ├─ gzip 解压（tinfl 复用）
  │   ├─ 和风 daily 7 天预报
  │   └─ gzip 解压
  └─ 页面绘制
```

## 关键技术点

### 1. JWT 鉴权（和风天气 API）

和风天气要求 API 请求携带 **Ed25519 签名的 JWT**。本项目的实现：

- 用 Ed25519 私钥对 JWT header + payload 签名
- 生成 base64url 编码的完整 JWT
- 通过 `Authorization: Bearer <jwt>` 头传给 API

### 2. gzip 流式解压

和风 API 返回 gzip 压缩数据，为在 ESP32 有限内存下解压：

- 使用 `tinfl_decompressor_alloc()` 在**堆上**分配解压器（约 10KB）
- **全局复用同一个解压器**，避免反复 malloc/free 造成堆碎片
- 加 `TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF` 标志，输出到连续大 buffer

### 3. HTTPS 连接复用

两次 HTTPS 请求（current + daily）复用同一个 `WiFiClientSecure`，避免 TLS 上下文重复分配造成的 `code=-1` 错误。

## 目录结构

```
sketch_sep25a/
├── sketch_sep25a.ino   # 主程序
├── miniz.h             # miniz 头文件
├── miniz.c             # miniz 实现
├── edsign.h            # Ed25519 签名接口
├── edsign.c
├── ed25519.c
├── c25519.c
├── f25519.c
├── fprime.c
├── morph25519.c
└── sha512.c
```

## 常见问题

**Q: 编译报 `undefined reference to tinfl_decompress_mem_to_mem`？**

A: 确保 `miniz.h` / `miniz.c` 在 sketch 目录下，且未被 Arduino 库管理器里的其他 miniz 覆盖。

**Q: 天气显示"无数据"？**

A: 检查 WiFi 是否连接、和风 API 凭据是否正确、私钥是否匹配控制台的公钥。

**Q: 预报接口报 `code=-1`？**

A: 堆内存不足导致 TLS 握手失败。确保 `g_secureClient` 是全局复用的，且两次 HTTPS 请求之间有 `delay(100)`。

**Q: JWT 生成失败？**

A: 检查 NTP 是否同步成功（JWT 依赖系统时间），以及 `ED25519_PRIVATE_KEY` 是否正确填写。

**Q: 农历不显示？**

A: 检查接口盒子的 `LUNAR_API_ID` 和 `LUNAR_API_KEY` 是否配置正确，以及 API 额度是否用完。

## 致谢

- **Ed25519 签名**：本项目使用的 Ed25519 签名实现来自
  [iot-tor/esp32-ed25519](https://github.com/iot-tor/esp32-ed25519)，
  用于生成和风天气 API 的 JWT 签名。感谢原作者的开源贡献。
- **miniz**：gzip/deflate 压缩库，作者 Rich Geldreich。
- **和风天气**：提供实时天气和 7 天预报 API。
- **apihz.cn**：提供农历 API。

## 第三方组件许可

- **Ed25519 签名**：来自 [iot-tor/esp32-ed25519](https://github.com/iot-tor/esp32-ed25519)，
  该仓库**未声明许可证**。本项目仅以引用方式使用其源码，
  不改变原项目的许可状态，使用者需自行评估合规性。
- **miniz**：MIT / Public Domain
- **Adafruit GFX / SSD1306**：BSD
- **ArduinoJson**：MIT
- **NTPClient**：MIT
- **U8g2_for_Adafruit_GFX**：MIT
- **和风天气 API**：受和风服务条款约束
- **apihz.cn 农历 API**：受 apihz 服务条款约束

## 注意事项

- 本项目中的 API 凭据（和风私钥、API Key 等）**请勿提交到公开仓库**。
  建议将配置抽到独立的 `secrets.h` 文件并加入 `.gitignore`。
- 和风天气的免费额度有限，建议合理设置更新间隔（默认 10 分钟）。
- 首次启动若未配置 WiFi，会进入配网模式，通过按键输入 SSID 和密码。

## License

本项目代码采用 MIT License 发布（详见 [LICENSE](LICENSE) 文件）。
