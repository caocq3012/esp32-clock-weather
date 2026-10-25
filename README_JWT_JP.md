# esp32_clock_and_weather JWT 文字盤なし版 特別説明

---

[简体中文](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT.md)

[English](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT_EN.md)

[日本語](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT_JP.md)

---

ESP32 ベースのデスクトップスマートクロック。OLED 表示、リアルタイム天気、7 日間予報、旧暦、NTP 時刻同期、WiFi プロビジョニング、時報などの機能をサポートします。

## 機能特性

- ⏰ **時計表示**：大きなフォントの時刻 + 日付 + 曜日 + 旧暦
- 🌤 **リアルタイム天気**：自動 IP 位置情報、都市・天気・気温・湿度を表示
- 📅 **7 日間予報**：QWeather daily API、アイコン + 日付
- 🌙 **旧暦表示**：apihz.cn API 経由で旧暦日付を取得
- 📡 **NTP 時刻同期**：5 分ごとに自動同期
- 🔌 **WiFi プロビジョニング**：SSID とパスワードのスキャン / 手動入力に対応
- 🔔 **時報**：ブザーが正時に鳴る（オン/オフ切替可能）
- 🌙 **夜間画面消灯**：21:00～07:00 は 10 秒後に自動消灯して省電力、down キーを押すと 10 秒間点灯、日中は常時点灯
- 💾 **オフラインキャッシュ**：天気データをローカルにキャッシュ、ネットワーク切断時でも表示可能
- 🔐 **高いセキュリティ**：JWT 認証を使用、API キーよりもセキュリティが高い

## ハードウェア要件

| コンポーネント | 型番 / パラメータ                         |
| -------------- | ----------------------------------------- |
| メイン制御     | ESP32（ESP32-DevKitC または互換ボード）   |
| ディスプレイ   | SSD1306 OLED 128×64（I2C、アドレス 0x3C） |
| ブザー         | パッシブ/アクティブブザー                 |
| ボタン         | タクトスイッチ 4 個（上/下/左/右）        |

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

**ローカルファイル**（ソースコードと同じ階層、直接パッケージでダウンロード可能）：

- `miniz.h` / `miniz.c` — gzip 解凍用
- `ed25519.h` / `fe.c` / `ge.c` / `sc.c` / `sha512.c` / `keypair.c` / `sign.c` / `verify.c` など — Ed25519 署名（orlp/ed25519、zlib ライセンス）

## API 申請

### QWeather API

1. [QWeather コンソール](https://console.qweather.com/)で登録
2. プロジェクトを作成し、以下を取得：
   - `QWEATHER_USER_ID`（開発者 ID）
   - `QWEATHER_API_HOST`（API Host、`xxx.re.qweatherapi.com` の形式）
   - 右上のアバター -- 設定から取得

### Ed25519 鍵ペアの生成

QWeather は JWT 署名に Ed25519 アルゴリズムを使用するため、鍵ペアを生成する必要があります。

**方法 1：OpenSSL を使用（推奨）**

```bash
openssl genpkey -algorithm ED25519 -out ed25519-private.pem && openssl pkey -pubout -in ed25519-private.pem > ed25519-public.pem
```

2 つのファイルが生成されます：

- `ed25519-private.pem` — 秘密鍵（自己保管、漏洩しないこと）
- `ed25519-public.pem` — 公開鍵（QWeather コンソールにアップロード）

**方法 2：QWeather 公式 JWT ツールを使用**

[https://jwt.qweather.com](https://jwt.qweather.com) にアクセスすると、ページが自動的に鍵ペアを生成し、秘密鍵と公開鍵を直接コピーできます。

**方法 3：QWeather 公式サンプルコードを使用**

QWeather 公式ドキュメントでは、Java、Python、Node.js などの言語の JWT 生成サンプルコードが提供されています。対応する言語の暗号ライブラリを参考に Ed25519 鍵ペアを生成できます。

### 公開鍵のアップロードと認証情報の取得

1. QWeather コンソールにログインし、**プロジェクト管理**に移動
2. プロジェクトを選択（なければ新規作成）、**「認証情報を追加」**をクリック
3. 認証情報名は任意
4. 認証方式で **「JSON Web Token」** を選択
5. `ed25519-public.pem` の全内容（`-----BEGIN PUBLIC KEY-----` と `-----END PUBLIC KEY-----` を含む）をコピーし、公開鍵入力欄に貼り付け
6. 保存後、**認証情報 ID** と **プロジェクト ID** を記録（後で**必要になります**）

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

**使い方**：

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

**出力サンプル**

```
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
...
```

**自分で手動で `{ }` と改行を追加してください。**

---

## 方法 3：QWeather 公式 JWT ツールを使用

[https://jwt.qweather.com](https://jwt.qweather.com) にアクセス

- ページに**32 バイトの 16 進数形式**の秘密鍵が直接表示されます（例：`c1b5c77b...`）
- 2 文字ごとに `0x` と `,` を追加すれば配列形式になります

**手書きは大変なので、方法 1 か 2 を使いましょう。**

### 接口盒子 API

1. [接口盒子](https://www.apihz.cn/)で登録
2. 以下を取得：
   - `LUNAR_API_ID`（開発者 ID）
   - `LUNAR_API_KEY`（API Key）

## クイックスタート

1. **このリポジトリをクローン / ダウンロード**して Arduino sketch ディレクトリに配置。

2. **ユーザー設定エリア**を変更：

   ```cpp
   // QWeather 設定
   const char* QWEATHER_USER_ID       = "QWeather 開発者 ID を入力";
   const char* QWEATHER_PROJECT_ID    = "QWeather プロジェクト ID を入力";
   const char* QWEATHER_CREDENTIAL_ID = "QWeather 認証情報 ID を入力";
   const char* QWEATHER_API_HOST      = "QWeather API host を入力";
   
   static const uint8_t ED25519_PRIVATE_KEY[32] = {
     // 生成した秘密鍵（32 バイト 16 進数配列）を入力
   };
   
   // 接口盒子設定
   const char* LUNAR_API_ID  = "接口盒子開発者 ID を入力";
   const char* LUNAR_API_KEY = "接口盒子 API key を入力";
   ```

3. **コンパイル・書き込み**。初回起動時は保存済みの WiFi に接続し、未保存の場合はプロビジョニングフローに入ります。

4. **ボタン操作**：

   - 時計ページ：左/右でページ切替、上でメニューへ
   - メニューページ：上/下で選択、右で確認、左で戻る
   - 左/右長押し：時報のオン/オフ

5. **⚠注意** コンパイル時に**ツール -- Partition Scheme**で**Huge APP (3MB No OTA/1MB SPIFFS)**を選択しないと、正常にコンパイル・アップロードできません！！

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

## 主要技術ポイント

### 1. JWT 認証（QWeather API）

QWeather は API リクエストに **Ed25519 署名付き JWT** を要求します。本プロジェクトの実装：

- 32 バイトの seed から `ed25519_create_keypair()` で公開鍵（32B）と秘密鍵（64B）を生成
- `ed25519_sign()` で `header.payload` に署名
- base64url エンコードして完全な JWT を組み立て、`Authorization: Bearer <jwt>` ヘッダーで API に送信

### 2. gzip ストリーミング解凍

QWeather API は gzip 圧縮データを返します。ESP32 の限られたメモリで解凍するため：

- `tinfl_decompressor_alloc()` で**ヒープ上**に解凍器を割り当て（約 10KB）
- **グローバルで同じ解凍器を再利用**し、malloc/free の繰り返しによるヒープ断片化を回避
- `TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF` フラグを付けて、連続した大きなバッファに出力

### 3. HTTPS 接続の再利用

2 回の HTTPS リクエスト（current + daily）で同じ `WiFiClientSecure` を再利用し、TLS コンテキストの重複割り当てによる `code=-1` エラーを回避。

## ディレクトリ構造

```
sketch_sep25a/
├── sketch_sep25a.ino   # メインプログラム
├── miniz.h             # miniz ヘッダー
├── miniz.c             # miniz 実装
├── ed25519.h           # Ed25519 インターフェース（orlp/ed25519）
├── fixedint.h          # 固定長整数型
├── fe.c / fe.h         # 体演算
├── ge.c / ge.h         # 群演算
├── sc.c / sc.h         # スカラー演算
├── sha512.c / sha512.h # SHA-512
├── keypair.c           # 鍵生成
├── sign.c              # 署名
├── verify.c            # 検証
└── precomp_data.h      # 事前計算テーブル
```

## よくある質問

**Q: コンパイル時に `undefined reference to tinfl_decompress_mem_to_mem` エラー？**

A: `miniz.h` / `miniz.c` が sketch ディレクトリにあり、Arduino ライブラリマネージャーの他の miniz に上書きされていないことを確認してください。

**Q: コンパイル時に `'edsign_sec_to_pub' was not declared in this scope` エラー？**

A: 本プロジェクトは `orlp/ed25519` に変更されており、`edsign` 系関数は使用していません。`ed25519.h` および `fe.c` / `ge.c` / `sc.c` / `sha512.c` / `keypair.c` / `sign.c` / `verify.c` がすべて sketch ディレクトリにあり、コード内で `ed25519_create_keypair()` と `ed25519_sign()` を呼び出していることを確認してください。

**Q: 天気が「データなし」と表示される？**

A: WiFi が接続されているか、QWeather API 認証情報が正しいか、秘密鍵がコンソールの公開鍵と一致しているかを確認してください。

**Q: 予報 API が `code=-1` を返す？**

A: ヒープメモリ不足により TLS ハンドシェイクが失敗しています。`g_secureClient` がグローバルで再利用され、2 回の HTTPS リクエスト間に `delay(100)` があることを確認してください。

**Q: JWT 生成が失敗する？**

A: NTP 同期が成功しているか（JWT はシステム時刻に依存）、`ED25519_PRIVATE_KEY` が正しく入力されているかを確認してください。

**Q: 旧暦が表示されない？**

A: 接口盒子の `LUNAR_API_ID` と `LUNAR_API_KEY` が正しく設定されているか、API の割り当てを使い切っていないかを確認してください。

## 謝辞

- **Ed25519 署名**：本プロジェクトは [orlp/ed25519](https://github.com/orlp/ed25519)
  （zlib ライセンス、Copyright (c) 2015 Orson Peters）を使用して JWT 署名を実装しています。原作者のオープンソースへの貢献に感謝します。
- **miniz**：gzip/deflate 圧縮ライブラリ、作者 Rich Geldreich。
- **QWeather**：リアルタイム天気と 7 日間予報 API を提供。
- **apihz.cn**：旧暦 API を提供。

## サードパーティコンポーネントライセンス

- **Ed25519 署名**：[orlp/ed25519](https://github.com/orlp/ed25519)、zlib ライセンス。
  使用時はその著作権表示とライセンス全文を保持する必要があり、変更したソースコードには「変更済み」と明記する必要があります。
- **miniz**：MIT / Public Domain
- **Adafruit GFX / SSD1306**：BSD
- **ArduinoJson**：MIT
- **NTPClient**：MIT
- **U8g2_for_Adafruit_GFX**：MIT
- **QWeather API**：QWeather のサービス利用規約に従います
- **apihz.cn 旧暦 API**：apihz のサービス利用規約に従います

サードパーティ依存関係の完全なライセンス声明は [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) を参照してください。

## 注意事項

- 本プロジェクトの API 認証情報（QWeather 秘密鍵、API Key など）は**公開リポジトリにコミットしないでください**。
  設定を独立した `secrets.h` ファイルに抽出し、`.gitignore` に追加することを推奨します。
- QWeather の無料枠は限られているため、更新間隔を適切に設定することを推奨します（デフォルト 10 分）。
- 初回起動時に WiFi が未設定の場合、プロビジョニングモードに入り、ボタンで SSID とパスワードを入力します。

## License

本プロジェクトのコードは MIT License で公開されています（詳細は [LICENSE](LICENSE) ファイルを参照）。
