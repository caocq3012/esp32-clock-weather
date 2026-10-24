# esp32_clock_and_weather JWT 文字盤なし版 特別説明

ESP32 ベースのデスクトップスマートクロック。OLED 表示、リアルタイム天気、7 日間予報、旧暦、NTP 時刻同期、WiFi プロビジョニング、時報などの機能をサポートします。

## 機能特性

- ⏰ **時計表示**：大きいフォントの時刻 + 日付 + 曜日 + 旧暦
- 🌤 **リアルタイム天気**：自動 IP 位置情報、都市、天気、気温、湿度を表示
- 📅 **7 日間予報**：QWeather daily API、アイコン + 日付
- 🌙 **旧暦表示**：apihz.cn API 経由で旧暦日付を取得
- 📡 **NTP 時刻同期**：5 分ごとに自動同期
- 🔌 **WiFi プロビジョニング**：スキャン / SSID とパスワードの手動入力に対応
- 🔔 **時報**：ブザーが正時に鳴る（オン/オフ切替可能）
- 🌙 **夜間消灯**：21:00～07:00 は 10 秒後に自動で画面を消灯して省電力、down キーを押すと 10 秒間点灯、日中は常時点灯
- 💾 **オフラインキャッシュ**：天気データをローカルにキャッシュし、オフライン時でも表示可能
- 🔐**高いセキュリティ**：JWT 認証を使用し、API key より安全性が高い

## ハードウェア要件

| コンポーネント | 型番 / パラメータ                         |
| -------------- | ----------------------------------------- |
| メイン制御     | ESP32（ESP32-DevKitC または互換ボード）   |
| ディスプレイ   | SSD1306 OLED 128×64（I2C、アドレス 0x3C） |
| ブザー         | パッシブ/アクティブブザー                 |
| ボタン         | 4 つのタクトスイッチ（上/下/左/右）       |

### ピン接続

| 機能         | GPIO |
| ------------ | ---- |
| OLED SDA     | 19   |
| OLED SCL     | 18   |
| ブザー       | 23   |
| ボタン UP    | 27   |
| ボタン DOWN  | 26   |
| ボタン LEFT  | 25   |
| ボタン RIGHT | 33   |

## 依存ライブラリ

Arduino IDE のライブラリマネージャーでインストール：

- `Adafruit GFX Library`
- `Adafruit SSD1306`
- `ArduinoJson`（7.x）
- `NTPClient`
- `U8g2_for_Adafruit_GFX`
- `WiFi`（ESP32 標準搭載）
- `HTTPClient`（ESP32 標準搭載）
- `WiFiClientSecure`（ESP32 標準搭載）
- `Preferences`（ESP32 標準搭載）

**ローカルファイル**（ソースコードと同じ階層に配置、そのままパッケージしてダウンロード可能）：

- `miniz.h` / `miniz.c` — gzip 解凍用
- `edsign.h` / `edsign.c` など — Ed25519 署名

## API 申請

### QWeather API

1. [QWeather コンソール](https://console.qweather.com/)に登録
2. プロジェクトを作成し、以下を取得：
   - `QWEATHER_USER_ID`（開発者 ID）
   - `QWEATHER_API_HOST`（API Host、`xxx.re.qweatherapi.com` の形式）
   - 右上のアバター -- 設定で取得

### Ed25519 鍵ペアの生成

QWeather は JWT 署名に Ed25519 アルゴリズムを使用するため、鍵ペアを生成する必要があります。

**方法 1：OpenSSL を使用（推奨）**

```bash
openssl genpkey -algorithm ED25519 -out ed25519-private.pem && openssl pkey -pubout -in ed25519-private.pem > ed25519-public.pem
```

2 つのファイルが生成されます：

- `ed25519-private.pem` — 秘密鍵（自分で保管し、漏洩しないでください）
- `ed25519-public.pem` — 公開鍵（QWeather コンソールにアップロード）

**方法 2：QWeather 公式 JWT ツールを使用**

[https://jwt.qweather.com](https://jwt.qweather.com)にアクセスすると、ページが自動的に鍵ペアを生成し、秘密鍵と公開鍵を直接コピーできます。

**方法 3：QWeather 公式サンプルコードを使用**

QWeather 公式ドキュメントでは、Java、Python、Node.js などの言語の JWT 生成サンプルコードが提供されています。対応する言語の暗号ライブラリを参考にして Ed25519 鍵ペアを生成できます。

### 公開鍵のアップロードと認証情報の取得

1. QWeather コンソールにログインし、**プロジェクト管理**に進む
2. プロジェクトを選択（なければ新規作成）、**「認証情報を追加」**をクリック
3. 認証情報名は任意
4. 認証方式で**「JSON Web Token」**を選択
5. `ed25519-public.pem` の全内容（`-----BEGIN PUBLIC KEY-----` と `-----END PUBLIC KEY-----` を含む）をコピーし、公開鍵入力欄に貼り付け
6. 保存後、**認証情報 ID** と **プロジェクト ID** を記録する（後で**必要になります**）

### 秘密鍵を 16 進数配列に変換

#### 方法 1：Python スクリプト（推奨）

**前提**：`cryptography` ライブラリをインストール

```bash
pip install cryptography
```

**スクリプト**：

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

**使用方法**：

```bash
python convert.py
```

**出力例**：

```
static const uint8_t ED25519_PRIVATE_KEY[32] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
```

**そのままコピーしてコードに貼り付けてください。**

---

## 方法 2：OpenSSL ワンライナー

```bash
openssl pkey -in ed25519-private.pem -outform DER | tail -c 32 | xxd -p -c 8 | sed 's/../0x&, /g'
```

**出力例**

```
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
...
```

**自分で `{ }` と改行を手動で追加してください。**

---

## 方法 3：QWeather 公式 JWT ツールを使用

[https://jwt.qweather.com](https://jwt.qweather.com)にアクセス

- ページに**32 バイトの 16 進数形式**の秘密鍵が直接表示されます（例：`c1b5c77b...`）
- 2 文字ごとに `0x` と `,` を追加すれば配列形式になります

**手書きは大変なので、方法 1 または 2 を使用してください。**

### インターフェースボックス API

1. [インターフェースボックス](https://www.apihz.cn/)に登録
2. 以下を取得：
   - `LUNAR_API_ID`（開発者 ID）
   - `LUNAR_API_KEY`（API Key）

## クイックスタート

1. このリポジトリを Arduino sketch ディレクトリに**クローン / ダウンロード**します。

2. **ユーザー設定エリア**を変更します：

   ```cpp
   // QWeather 設定
   const char* QWEATHER_USER_ID       = "QWeather 開発者 ID を入力";
   const char* QWEATHER_PROJECT_ID    = "QWeather プロジェクト ID を入力";
   const char* QWEATHER_CREDENTIAL_ID = "QWeather 認証情報 ID を入力";
   const char* QWEATHER_API_HOST      = "QWeather API host を入力";
   
   static const uint8_t ED25519_PRIVATE_KEY[32] = {
     // 生成した秘密鍵（32 バイトの 16 進数配列）を入力
   };
   
   // インターフェースボックス設定
   const char* LUNAR_API_ID  = "インターフェースボックス開発者 ID を入力";
   const char* LUNAR_API_KEY = "インターフェースボックス API key を入力";
   ```

3. **コンパイルして書き込み**。初回起動時は保存済みの WiFi に接続し、保存されていない場合はプロビジョニングフローに入ります。

4. **ボタン操作**：

   - 時計ページ：左/右でページ切替、上でメニューに入る
   - メニューページ：上/下で選択、右で確定、左で戻る
   - 左/右長押し：時報のオン/オフ

5. **⚠️注** コンパイル時に**ツール -- Partition Scheme**で**Huge APP (3MB No OTA/1MB SPIFFS)**を選択する必要があります。そうしないと正常にコンパイル・アップロードできません！！！注意

## アーキテクチャ概要

```
setup()
  ├─ OLED / u8g2 初期化
  ├─ prefs 初期化
  ├─ tinfl 解凍器の事前割り当て（ヒープ上 10KB）
  ├─ WiFi 接続
  ├─ NTP 同期
  └─ 起動アニメーション

loop()
  ├─ ボタンスキャン
  ├─ WiFi 再接続
  ├─ NTP 定期同期（5 分）
  ├─ 天気更新（10 分）
  │   ├─ ip9 位置情報
  │   ├─ QWeather current（JWT + Ed25519 認証）
  │   ├─ gzip 解凍（tinfl 再利用）
  │   ├─ QWeather daily 7 日間予報
  │   └─ gzip 解凍
  └─ ページ描画
```

## 主要な技術ポイント

### 1. JWT 認証（QWeather API）

QWeather は API リクエストに **Ed25519 署名付き JWT** を要求します。本プロジェクトの実装：

- Ed25519 秘密鍵で JWT header + payload に署名
- base64url エンコードされた完全な JWT を生成
- `Authorization: Bearer <jwt>` ヘッダーで API に送信

### 2. gzip ストリーミング解凍

QWeather API は gzip 圧縮データを返します。ESP32 の限られたメモリで解凍するために：

- `tinfl_decompressor_alloc()` を使用して**ヒープ上**に解凍器を割り当て（約 10KB）
- **グローバルに同じ解凍器を再利用**し、malloc/free の繰り返しによるヒープ断片化を回避
- `TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF` フラグを付けて、連続した大きなバッファに出力

### 3. HTTPS 接続の再利用

2 回の HTTPS リクエスト（current + daily）で同じ `WiFiClientSecure` を再利用し、TLS コンテキストの重複割り当てによる `code=-1` エラーを回避します。

## ディレクトリ構造

```
sketch_sep25a/
├── sketch_sep25a.ino   # メインプログラム
├── miniz.h             # miniz ヘッダーファイル
├── miniz.c             # miniz 実装
├── edsign.h            # Ed25519 署名インターフェース
├── edsign.c
├── ed25519.c
├── c25519.c
├── f25519.c
├── fprime.c
├── morph25519.c
└── sha512.c
```

## よくある質問

**Q: コンパイル時に `undefined reference to tinfl_decompress_mem_to_mem` が発生する？**

A: `miniz.h` / `miniz.c` が sketch ディレクトリにあること、および Arduino ライブラリマネージャーの他の miniz に上書きされていないことを確認してください。

**Q: 天気が「データなし」と表示される？**

A: WiFi が接続されているか、QWeather API 認証情報が正しいか、秘密鍵がコンソールの公開鍵と一致しているかを確認してください。

**Q: 予報 API が `code=-1` を返す？**

A: ヒープメモリ不足により TLS ハンドシェイクが失敗しています。`g_secureClient` がグローバルに再利用されていること、および 2 回の HTTPS リクエスト間に `delay(100)` があることを確認してください。

**Q: JWT 生成に失敗する？**

A: NTP 同期が成功しているか（JWT はシステム時刻に依存）、および `ED25519_PRIVATE_KEY` が正しく入力されているかを確認してください。

**Q: 旧暦が表示されない？**

A: インターフェースボックスの `LUNAR_API_ID` と `LUNAR_API_KEY` が正しく設定されているか、API の利用枠が使い切っていないかを確認してください。

## 謝辞

- **Ed25519 署名**：本プロジェクトで使用している Ed25519 署名実装は
  [iot-tor/esp32-ed25519](https://github.com/iot-tor/esp32-ed25519) から提供されており、
  QWeather API の JWT 署名生成に使用しています。原作者のオープンソースへの貢献に感謝します。
- **miniz**：gzip/deflate 圧縮ライブラリ、作者 Rich Geldreich。
- **QWeather**：リアルタイム天気と 7 日間予報 API を提供。
- **apihz.cn**：旧暦 API を提供。

## サードパーティコンポーネントライセンス

- **Ed25519 署名**：[iot-tor/esp32-ed25519](https://github.com/iot-tor/esp32-ed25519) から提供されており、
  このリポジトリは**ライセンスを宣言していません**。本プロジェクトはそのソースコードを参照方式でのみ使用し、
  元プロジェクトのライセンス状態を変更せず、使用者は自身でコンプライアンスを評価する必要があります。
- **miniz**：MIT / Public Domain
- **Adafruit GFX / SSD1306**：BSD
- **ArduinoJson**：MIT
- **NTPClient**：MIT
- **U8g2_for_Adafruit_GFX**：MIT
- **QWeather API**：QWeather 利用規約に従います
- **apihz.cn 旧暦 API**：apihz 利用規約に従います

## 注意事項

- 本プロジェクトの API 認証情報（QWeather 秘密鍵、API Key など）は**公開リポジトリにコミットしないでください**。
  設定を独立した `secrets.h` ファイルに抽出し、`.gitignore` に追加することを推奨します。
- QWeather の無料枠は限られているため、更新間隔を適切に設定することを推奨します（デフォルト 10 分）。
- 初回起動時に WiFi が未設定の場合、プロビジョニングモードに入り、ボタンで SSID とパスワードを入力します。

## License

本プロジェクトのコードは MIT License で公開されています（詳細は [LICENSE](LICENSE) ファイルを参照）。
