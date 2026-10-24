# ESP32-OLED-天気時計

ESP32 128×64 OLED I2C スマートデスクトップ天気時計。中国語表示、旧暦、二十四節気、QWeather V1 APIによる現在の天気＋7日間予報の自動取得、ボタンによるWiFiプロビジョニング、オフラインキャッシュ、夜間の画面消灯による省エネ、毎正時のブザー時報に対応。

---

[JWT文字盤なし版の特別説明](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT.md)

## 📌 プロジェクト概要

ESP32開発ボードをベースにしたOLEDデスクトップ時計で、SSD1306 128×64 I2C OLEDスクリーンに対応。

主な機能：

- **NTP自動ネットワーク時刻同期**：起動時の即時同期＋定期的な更新に対応、オフラインでも動作継続可能
- **旧暦＋二十四節気**：apihz.cn経由で旧暦の月日・節気情報をオンライン取得
- **IP自動位置情報**：ip9.com.cn経由で緯度経度と都市名を取得
- **QWeather V1 API**：現在の天気＋7日間予報、gzip手動解凍（miniz）付き
- **WiFiプロビジョニング**：スキャンリストからの視覚的なネットワーク選択、パスワード文字入力、XOR暗号化してPreferencesに保存
- **4ボタン操作**：ページ切替、メニュー選択、WiFiパスワード入力、長押し確定
- **毎正時ブザー時報**：オン/オフ切替対応
- **夜間自動画面消灯**：21:00～07:00は画面消灯、downキーを押すと10秒間点灯、日中は常時点灯
- **オフラインキャッシュ**：ネット切断時は過去の天気キャッシュを読み込み
- **手描き天気アイコン**：晴れ/曇り/雨/雷/雪/霧/もや/砂塵など
- **シリアルハートビートログ**：デバッグ用

---

## 🧰 ハードウェアリスト

| ハードウェア | 型番/仕様                                                                                                                                                                    |
| ------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| マイコン     | ESP32開発ボード（任意の型番、Flash ≥ 4MB）                                                                                                                                   |
| 画面         | SSD1306、128×64、I2C、アドレス `0x3C`淘宝リンク（ログイン必要）：https://item.taobao.com/item.htm?id=767205498137&skuId=5440471327277    、仕様は4ピンIIC OLED液晶屏を選択） |
| ボタン ×4    | タクトスイッチ                                                                                                                                                               |
| ブザー       | 有源または無源、GPIO 23に接続                                                                                                                                                |

tips：ブザーを接続すると鳴りっぱなしになる原因が分かりません。分かる方は見ていただけると幸いです。分からない場合は接続しない方が良いでしょう

### ピン定義

| 周辺機器           | GPIO    |
| ------------------ | ------- |
| OLED SDA           | GPIO 19 |
| OLED SCL           | GPIO 18 |
| ブザー BEEP        | GPIO 23 |
| KEY_UP 上ボタン    | GPIO 27 |
| KEY_DOWN 下ボタン  | GPIO 26 |
| KEY_LEFT 左ボタン  | GPIO 25 |
| KEY_RIGHT 右ボタン | GPIO 33 |

> ⚠️ OLEDの電源は3.3Vを推奨。画面破損を避けるため5Vは接続しないでください。I2Cアドレスはデフォルト `0x3C`、画面が真っ暗な場合は `0x3D` に変更してみてください。

---

## 📦 依存ライブラリ

**Arduino IDE → ライブラリマネージャから以下をインストール：**

| ライブラリ名            | 作者             | 用途                              |
| ----------------------- | ---------------- | --------------------------------- |
| `Adafruit GFX Library`  | Adafruit         | グラフィック描画基盤ライブラリ    |
| `Adafruit SSD1306`      | Adafruit         | OLEDスクリーンドライバ            |
| `U8g2_for_Adafruit_GFX` | olikraus         | OLED中国語フォント（GB2312）      |
| `NTPClient`             | Fabrice Weinberg | NTP時刻同期                       |
| `ArduinoJson`           | Benoit Blanchon  | JSON解析（**6.xバージョン必須**） |

**プロジェクトファイルに同梱：**

- `miniz.c` / `miniz.h` —— gzip解凍（tinfl解凍を手動実装）

**プロジェクトディレクトリに配置（`.ino` と同じ階層）：**

```
esp32_clock_and_weather/
├── sketch_sep25a.ino
├── miniz.c
└── miniz.h
```

---

## ⚙️ コンパイル設定

Arduino IDE → **ツール → Partition Scheme** → 選択：

```
Huge APP (3MB No OTA/1MB SPIFFS)
```

**そうしないとコンパイルに失敗します**（中国語＋SSL＋minizがデフォルトの1.3MBアプリパーティションを超えるため）。

---

## 🔑 ユーザー設定

`.ino` ファイルを開き、「ユーザー設定エリア」を見つけてご自身のアカウント情報を入力してください：

```cpp
// QWeather（https://console.qweather.com/）
const char* QWEATHER_API_KEY  = "QWeatherのapi-keyを入力";
const char* QWEATHER_API_HOST = "QWeatherのapi-hostを入力";

// apihz（https://www.apihz.cn/）
const char* LUNAR_API_ID  = "apihzの開発者IDを入力";
const char* LUNAR_API_KEY = "apihzの開発者API-keyを入力";
```

### QWeather設定

1. https://console.qweather.com/ に登録
2. 「プロジェクト管理」→ プロジェクト作成
3. 「認証情報」→ API Keyを生成
4. 「設定」→ API Hostを確認

### apihz設定

1. https://www.apihz.cn/ に登録
2. 個人センターで **開発者ID** と **API Key** を確認できます
3. コードに入力

---

## 🎮 ボタン操作

| ボタン          | 時計/天気ページ       | メニューページ   | WiFiスキャン   | パスワード入力   |
| --------------- | --------------------- | ---------------- | -------------- | ---------------- |
| **左**          | 時計ページに切替      | 戻る             | メニューに戻る | バックスペース   |
| **右**          | 天気ページに切替      | オプションに入る | SSIDを選択     | 文字を追加       |
| **上**          | メニューに入る        | オプションを上へ | リストを上へ   | 文字 +1          |
| **下**          | —                     | オプションを下へ | リストを下へ   | 文字 -1          |
| **左/右長押し** | 毎正時時報のオン/オフ | —                | —              | 長押しで接続確定 |

---

## 🔄 データ更新頻度

| データ              | 頻度                       |
| ------------------- | -------------------------- |
| NTP時刻             | 起動時に1回同期＋5分ごと   |
| 天気（現在＋7日間） | 起動時に1回取得＋10分ごと  |
| 旧暦＋二十四節気    | 1日1回（日付変更時に更新） |

---

## 📂 バージョン説明

本プロジェクトには3つのブランチがあります：

- **`main（本ブランチ）`**：両バージョンあり（JWT文字盤なし版を除く）
- **`文字盤版`**
- **`文字盤なし版`**
- **`JWT文字盤なし版`**

ブランチ切替：

GitHubページ左上のブランチドロップダウンで切替。

特記事項：文字盤なし版はAPIキーでQWeatherにアクセスし、JWT文字盤なし版はJWTでアクセスするためセキュリティは向上しますが設定が複雑です。ご自身で選択してください。JWT文字盤なし版の使用方法はJWT文字盤なし版ブランチのREADMEを参照してください。また、JWT文字盤なし版は必要なファイルが多いため、必要であればJWT文字盤なし版ブランチからダウンロードしてください。

## ⚠️ 注意事項

- **天気機能は中国国内でのみ利用可能**（ip9.com.cnの位置情報＋QWeather国内APIに依存）
- **旧暦機能はapihz APIに依存**、リクエスト頻度は低く（1日1回）、無料枠で十分
- **API Keyは公開しないでください**、他人にあなたの枠を使われます
- **WiFiパスワードやAPI Keyを平文でGitHubにpushしないでください**

---

## 🙏 謝辞

- 天気データ：[QWeather](https://www.qweather.com/)
- 旧暦API：[apihz](https://www.apihz.cn/)
- IP位置情報：[ip9.com.cn](http://ip9.com.cn/)
- gzip解凍：[miniz](https://github.com/richgel999/miniz)（richgel999）
- 中国語フォントレンダリング：[U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX)（olikraus）
- 時刻同期：[NTPClient](https://github.com/arduino-libraries/NTPClient)（Fabrice Weinberg）

---

## 📄 License

本プロジェクトは [MIT License](LICENSE) の下でオープンソース化されています。Copyright (c) 2026 gaocq。

### サードパーティコンポーネント

本プロジェクト（JWT文字盤なし版を除く）は以下のサードパーティオープンソースコンポーネントを使用しており、それぞれ元のライセンスに従います：

- **[miniz](https://github.com/richgel999/miniz)** —— MIT License
  Copyright 2013-2014 RAD Game Tools and Valve Software
  Copyright 2010-2014 Rich Geldreich and Tenacious Software LLC
- **Adafruit GFX / SSD1306**：BSD
- **ArduinoJson**：MIT
- **NTPClient**：MIT
- **U8g2_for_Adafruit_GFX**：MIT
- **QWeather API**：QWeather利用規約に準拠
- **apihz.cn 旧暦API**：apihz利用規約に準拠

---

## ⭐ このプロジェクトが役に立ったら、ぜひStarをお願いします

---
