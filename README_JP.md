# ESP32-OLED-Weather-Clock

ESP32 128×64 OLED I2C スマートデスクトップ天気時計。中国語表示、旧暦、二十四節気、QWeather V1 APIによる現在の天気と7日間予報の自動取得、ボタンによるWi-Fi設定、オフラインキャッシュ、夜間画面オフ省エネ、定時チャイム機能をサポート。

---

[JWTディスプレイなし版の特別説明](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT_JP.md)

[English](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_EN.md)

[简体中文](https://github.com/caocq3012/esp32-clock-weather/blob/main/README.md)

[日本語](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JP.md)

## 📌 プロジェクト概要

ESP32開発ボードをベースにしたOLEDデスクトップ時計で、SSD1306 128×64 I2C OLED画面に対応しています。

主な機能：

- **NTP自動時刻同期**：起動時に即時同期＋定期的な更新、オフラインでも動作継続可能
- **旧暦＋二十四節気**：apihz.cn経由で旧暦の月日、節気情報をオンライン取得
- **IP自動位置情報**：ip9.com.cn経由で緯度・経度・都市名を取得
- **QWeather V1 API**：現在の天気＋7日間予報、gzipの手動解凍（miniz）を含む
- **Wi-Fi設定**：ネットワークリストの可視化、パスワードの文字入力、XOR暗号化してPreferencesに保存
- **4ボタン操作**：ページ切り替え、メニュー選択、Wi-Fiパスワード入力、長押し確認
- **定時チャイム**：オン/オフ切り替え可能
- **夜間自動画面オフ**：21:00～07:00は画面オフ。DOWNキーを押すと10秒間画面が点灯、日中は常時点灯
- **オフラインキャッシュ**：ネットワーク切断時に履歴の天気キャッシュを読み込み
- **手書き風天気アイコン**：晴れ/曇り/雨/雷/雪/霧/ヘイズ/砂塵など
- **シリアルハートビートログ**：デバッグ用

---

## 🧰 ハードウェアリスト

| ハードウェア | 型番/パラメータ                                                                                                                                                          |
| ------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| マイコン     | ESP32開発ボード（任意のモデル、Flash ≥ 4MB）                                                                                                                             |
| 画面         | SSD1306、128×64、I2C、アドレス `0x3C` Taobaoリンク（ログイン必須）：https://item.taobao.com/item.htm?id=767205498137&skuId=5440471327277 、4ピンIIC OLED液晶画面を選択） |
| ボタン ×4    | タクトスイッチ                                                                                                                                                           |
| ブザー       | アクティブまたはパッシブ、GPIO 23に接続                                                                                                                                  |

ヒント：ブザーを接続すると鳴り止まらないのですが、原因が分かりません。分かる方がいれば教えてください。分からない場合は接続しない方が良いかもしれません。

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

> ⚠️ OLEDの電源は3.3Vを推奨します。画面の焼き付きを避けるため、5Vは接続しないでください。I2Cアドレスのデフォルトは`0x3C`です。画面が真っ暗な場合は`0x3D`に変更してみてください。

---

## 📦 依存ライブラリ

**Arduino IDE → ライブラリマネージャで以下のライブラリをインストール：**

| ライブラリ名            | 作者             | 用途                                |
| ----------------------- | ---------------- | ----------------------------------- |
| `Adafruit GFX Library`  | Adafruit         | グラフィック描画基本ライブラリ      |
| `Adafruit SSD1306`      | Adafruit         | OLED画面ドライバ                    |
| `U8g2_for_Adafruit_GFX` | olikraus         | OLED中国語フォント（GB2312）        |
| `NTPClient`             | Fabrice Weinberg | NTP時刻同期                         |
| `ArduinoJson`           | Benoit Blanchon  | JSON解析（**6.xバージョンが必要**） |

**プロジェクトに同梱されているファイル：**

- `miniz.c` / `miniz.h` —— gzip解凍（tinfl解凍を手動実装）

**プロジェクトディレクトリに配置（`.ino`と同じ階層）：**

esp32_clock_and_weather/
├── sketch_sep25a.ino
├── miniz.c
└── miniz.h

---

## ⚙️ コンパイル設定

Arduino IDE → **ツール → Partition Scheme** → 以下を選択：

Huge APP (3MB No OTA/1MB SPIFFS)

**そうしないとコンパイルに失敗します**（中国語 + SSL + minizがデフォルトの1.3MBアプリパーティションを超えるため）。

---

## 🔑 ユーザー設定

`.ino`ファイルを開き、「ユーザー設定エリア」を見つけて、ご自身のアカウント情報を入力してください：

// QWeather (https://console.qweather.com/)
const char* QWEATHER_API_KEY  = "QWeatherのapi-keyを入力";
const char* QWEATHER_API_HOST = "QWeatherのapi-hostを入力";

// apihz (https://www.apihz.cn/)
const char* LUNAR_API_ID  = "apihzの開発者IDを入力";
const char* LUNAR_API_KEY = "apihzの開発者API-keyを入力";

### QWeatherの設定

1. https://console.qweather.com/ で登録
2. 「プロジェクト管理」→ プロジェクト作成
3. 「認証情報」→ API Keyを生成
4. 「設定」→ API Hostを確認

### apihzの設定

1. https://www.apihz.cn/ で登録
2. パーソナルセンターで**開発者ID**と**API Key**を確認
3. コードに入力

---

## 🎮 ボタン操作

| ボタン          | 時計/天気ページ         | メニューページ | Wi-Fiスキャン  | パスワード入力 |
| --------------- | ----------------------- | -------------- | -------------- | -------------- |
| **左**          | 時計ページへ            | 戻る           | メニューへ戻る | バックスペース |
| **右**          | 天気ページへ            | オプションへ   | SSIDを選択     | 文字を追加     |
| **上**          | メニューへ              | オプション上へ | リスト上へ     | 文字 +1        |
| **下**          | —                       | オプション下へ | リスト下へ     | 文字 -1        |
| **左/右長押し** | 定時チャイムのオン/オフ | —              | —              | 長押しで接続   |

---

## 🔄 データ更新頻度

| データ               | 頻度                       |
| -------------------- | -------------------------- |
| NTP時刻              | 起動時1回 + 5分ごと        |
| 天気（現在 + 7日間） | 起動時1回 + 10分ごと       |
| 旧暦 + 二十四節気    | 1日1回（日付変更時に更新） |

---

## 📂 バージョン説明とダウンロード

すべてのバージョンは`main`ブランチに統合されています。**ブランチを切り替える必要はありません**。必要に応じて該当のフォルダをクリックしてダウンロードしてください：

- 📁 **`表盘版/sketch_sep25a`** —— 文字盤UI付きの完全版

- 📁 **`无表盘版/sketch_sep25a`** —— 推奨（文字盤表示が少し窮屈なため）

- 📁 **`无表盘JWT版/sketch_sep25a`** —— 高セキュリティ版（設定が複雑）[JWTディスプレイなし版の特別説明](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT_JP.md)

**💡 特記事項：**

- **ディスプレイなし版（文字盤版含む）**：通常のAPI Keyを使用してQWeatherにアクセスします。設定が簡単で初心者に最適です。

- **ディスプレイなしJWT版**：Ed25519アルゴリズムを使用してJWTを生成しQWeatherにアクセスします。セキュリティは大幅に向上しますが、設定は複雑です。

  - ⚠️ **注意**：JWT版は多数の低レベル暗号化依存ファイルを含みます。ダウンロード後、必ず[JWTディスプレイなし版の特別説明](https://github.com/caocq3012/esp32-clock-weather/blob/main/README_JWT_JP.md)をお読みいただき設定を行ってください。

**🔑 クイック設定ガイド（通常版）：**  
コードをダウンロード後、`.ino`ファイルを開き、冒頭の**「ユーザー設定エリア」**にWi-Fi、QWeather、apihzの情報を直接入力すれば、すぐにコンパイルして実行できます。より高いセキュリティが必要な場合は、同じ階層に`key.h`ファイルを作成して設定を入力することもできます。プログラムは自動的に`key.h`を優先的に読み込みます。

## ⚠️ 注意事項

- **天気機能は中国国内での使用のみサポート**（ip9.com.cnの位置情報 + QWeather国内APIに依存）
- **旧暦機能はapihz APIに依存**、リクエスト頻度は低い（1日1回）、無料枠で十分
- **API Keyは公開しないでください**、他人にあなたの枠を使われてしまいます
- **Wi-FiパスワードやAPI Keyを平文でGitHubにプッシュしないでください**

---

- 📁 **`text/sketch_oct24a`** —— おまけ：純粋なハードウェアテストプログラム（OLEDとボタンが正常に動作するかのテスト用）

---

## 🙏 謝辞

- 天気データ：[QWeather](https://www.qweather.com/)
- 旧暦API：[apihz](https://www.apihz.cn/)
- IP位置情報：[ip9.com.cn](http://ip9.com.cn/)
- gzip解凍：[miniz](https://github.com/richgel999/miniz)（richgel999）
- 中国語フォントレンダリング：[U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX)（olikraus）
- 時刻同期：[NTPClient](https://github.com/arduino-libraries/NTPClient)（Fabrice Weinberg）

---

## 📄 ライセンス

本プロジェクトは[MIT License](LICENSE)の下でオープンソース化されています。Copyright (c) 2026 gaocq。

### サードパーティコンポーネント

本プロジェクト（JWTディスプレイなし版を除く）は以下のサードパーティオープンソースコンポーネントを使用しており、それぞれ元のライセンスに従います：

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

## ⭐ このプロジェクトが役に立ったら、Starをお願いします

---
