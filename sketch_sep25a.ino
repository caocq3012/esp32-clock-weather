#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <WiFiClientSecure.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "miniz.h"

//==================== 用户配置区 =====================
const char* QWEATHER_API_KEY = "dd86273986a84d81af30e0bc6096defa";
const char* QWEATHER_API_HOST = "ke7fc4nn45.re.qweatherapi.com";

// ★ 接口盒子（https://www.apihz.cn/）注册后填这里
const char* LUNAR_API_ID  = "10021574";
const char* LUNAR_API_KEY = "814ad6b36c32c6905cc7b2818fc829a9";

//===================== 硬件配置 =====================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define BEEP_PIN 23
#define KEY_UP 27
#define KEY_DOWN 26
#define KEY_LEFT 25
#define KEY_RIGHT 33
#define OLED_SDA 19
#define OLED_SCL 18

const char* NTP_SERVER = "ntp.aliyun.com";
const long GMT_OFFSET = 8 * 3600;
const unsigned long WEATHER_INTERVAL = 600000UL;
const unsigned long NTP_INTERVAL = 300000UL;
const unsigned long NIGHT_SCREEN_TIMEOUT = 10000UL;
const unsigned long DAY_SCREEN_TIMEOUT = 60000UL;
const uint8_t WIFI_SCAN_MAX = 20;
#define WIFI_PASS_MAXLEN 64
const unsigned long WIFI_RECONNECT_INTERVAL = 10000UL;
const uint8_t XOR_KEY = 0x5A;
int startLine = 0;
const int maxShow = 4;

//===============================================================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
U8G2_FOR_ADAFRUIT_GFX u8g2;
WiFiUDP udp;
NTPClient timeClient(udp, NTP_SERVER, GMT_OFFSET, 60000);
Preferences prefs;

enum PageState { PAGE_CLOCK, PAGE_WEATHER, PAGE_MENU, PAGE_WIFI_SCAN, PAGE_WIFI_PASS, PAGE_WIFI_SETUP };
PageState currentPage = PAGE_CLOCK;
int menuSelection = 0;
int wifiSetupSelection = 0;
String wifiList[WIFI_SCAN_MAX];
int wifiScanCount = 0;
int wifiSelectIdx = 0;
String selectedSSID = "";
const char charSet[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()_-";
int charIndex = 0;
String inputPASS = "";

int year = 2026, mon = 1, mday = 1;
int hourNow = 0, minNow = 0, secNow = 0;
int lastMin = -1;
bool timeSynced = false;

char cityName[32] = "未知";
char weatherInfo[32] = "无数据";
float temp = 0;
int humidity = 0;
bool offlineMode = false;
struct ForecastDay { int month; int day; char text[16]; float tempMax; float tempMin; };
ForecastDay forecast[7];
int forecastCount = 0;
bool forecastLoaded = false;

bool hourBellEnable = true;
unsigned long screenActiveTimer = 0;
bool screenOn = true;
unsigned long lastWeatherUpdate = 0;
unsigned long lastNtpUpdate = 0;
unsigned long lastWifiReconnectTry = 0;
bool wifiManualConfig = false;

bool beepActive = false;
unsigned long beepStart = 0;
int beepCnt = 0, beepTotal = 0;
const unsigned long BEEP_ON = 300, BEEP_OFF = 200;
bool shortBeepActive = false;
bool showWifiTip = false;
unsigned long wifiTipStart = 0;
const unsigned long WIFI_TIP_DURATION = 800UL;

// ★ 农历缓存（每天只请求一次 API）
String cachedLunar = "";
int cachedLunarYear = -1;
int cachedLunarMon = -1;
int cachedLunarDay = -1;

//==================== 中文绘制 ====================
void drawCN(int x, int y, const char* text) {
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setForegroundColor(SSD1306_WHITE);
  u8g2.setBackgroundColor(SSD1306_BLACK);
  u8g2.setCursor(x, y);
  u8g2.print(text);
}
void drawCN(int x, int y, const String& text) { drawCN(x, y, text.c_str()); }
void setPage(PageState p) { currentPage = p; screenOn = true; screenActiveTimer = millis(); }

//==================== XOR ====================
void xorEncDec(char* buf, uint8_t key) { for (int i = 0; buf[i] != '\0'; i++) buf[i] ^= key; }
void saveWiFiCfg(String ssid, String pass) {
  char passBuf[128]; pass.toCharArray(passBuf, sizeof(passBuf));
  xorEncDec(passBuf, XOR_KEY);
  prefs.putString("ssid", ssid);
  prefs.putBytes("pass_enc", passBuf, strlen(passBuf) + 1);
}
String loadWiFiPass() {
  char buf[128] = {0};
  size_t len = prefs.getBytes("pass_enc", buf, sizeof(buf));
  if (len <= 0) return "";
  xorEncDec(buf, XOR_KEY);
  return String(buf);
}

//==================== 蜂鸣器 ====================
void startShortBeep() { screenActiveTimer = millis(); screenOn = true; shortBeepActive = true; beepStart = millis(); }
void startHourBeep(int count) {
  if (count <= 0) return;
  beepActive = true; beepCnt = 0; beepTotal = count; beepStart = millis();
  digitalWrite(BEEP_PIN, HIGH);
}
void beepTask() {
  unsigned long now = millis();
  if (shortBeepActive) {
    if (digitalRead(BEEP_PIN) == HIGH && now - beepStart >= 100) { digitalWrite(BEEP_PIN, LOW); shortBeepActive = false; }
    return;
  }
  if (!beepActive) return;
  if (digitalRead(BEEP_PIN) == HIGH) {
    if (now - beepStart >= BEEP_ON) { digitalWrite(BEEP_PIN, LOW); beepStart = now; }
  } else {
    if (now - beepStart >= BEEP_OFF) {
      beepCnt++;
      if (beepCnt >= beepTotal) { beepActive = false; return; }
      digitalWrite(BEEP_PIN, HIGH); beepStart = now;
    }
  }
}

//==================== 屏幕绘图 ====================
void drawWifiIcon(int rssi) {
  int bars = 0;
  if (WiFi.status() == WL_CONNECTED) {
    if (rssi >= -50) bars = 5; else if (rssi >= -60) bars = 4;
    else if (rssi >= -70) bars = 3; else if (rssi >= -80) bars = 2;
    else if (rssi >= -90) bars = 1; else bars = 0;
  } else bars = 0;
  int x = 116;   // ★ 右移到 116
  if (bars == 0) {
    display.drawLine(x + 1, 1, x + 7, 7, WHITE);
    display.drawLine(x + 7, 1, x + 1, 7, WHITE);
  } else {
    if (bars >= 3) display.drawLine(x, 6, x + 8, 6, WHITE);
    if (bars >= 2) display.drawLine(x + 2, 4, x + 6, 4, WHITE);
    if (bars >= 1) display.drawLine(x + 3, 2, x + 5, 2, WHITE);
    for (int i = 0; i < bars; i++) {
      int h = 1 + i;
      display.drawFastVLine(x + i * 2, 6 - h, h, WHITE);
    }
  }
}

// ★ 表盘函数已删除

void drawWeatherIcon(int x, int y, String cond) {
  cond.toLowerCase();
  if (cond.indexOf("晴") >= 0) display.drawCircle(x + 8, y + 8, 6, WHITE);
  else if (cond.indexOf("云") >= 0 || cond.indexOf("阴") >= 0) {
    display.drawCircle(x + 4, y + 6, 3, WHITE);
    display.drawCircle(x + 8, y + 4, 4, WHITE);
    display.drawCircle(x + 12, y + 6, 3, WHITE);
    display.drawRect(x + 1, y + 6, 14, 4, WHITE);
  } else if (cond.indexOf("雨") >= 0) {
    display.drawCircle(x + 4, y + 6, 3, WHITE);
    display.drawCircle(x + 8, y + 4, 4, WHITE);
    display.drawCircle(x + 12, y + 6, 3, WHITE);
    display.drawRect(x + 1, y + 6, 14, 4, WHITE);
    display.drawLine(x + 3, y + 12, x + 5, y + 15, WHITE);
    display.drawLine(x + 9, y + 12, x + 11, y + 15, WHITE);
  } else if (cond.indexOf("雷") >= 0) {
    display.drawCircle(x + 4, y + 6, 3, WHITE);
    display.drawCircle(x + 8, y + 4, 4, WHITE);
    display.drawCircle(x + 12, y + 6, 3, WHITE);
    display.drawRect(x + 1, y + 6, 14, 4, WHITE);
    display.drawLine(x + 6, y + 10, x + 4, y + 14, WHITE);
    display.drawLine(x + 6, y + 10, x + 8, y + 14, WHITE);
  } else if (cond.indexOf("雪") >= 0) {
    display.drawCircle(x + 4, y + 6, 3, WHITE);
    display.drawCircle(x + 8, y + 4, 4, WHITE);
    display.drawCircle(x + 12, y + 6, 3, WHITE);
    display.drawRect(x + 1, y + 6, 14, 4, WHITE);
    display.drawCircle(x + 3, y + 13, 1, WHITE);
    display.drawCircle(x + 8, y + 13, 1, WHITE);
    display.drawCircle(x + 13, y + 13, 1, WHITE);
  } else if (cond.indexOf("雾") >= 0) {
    display.drawLine(x + 2, y + 6, x + 8, y + 6, WHITE);
    display.drawLine(x + 4, y + 9, x + 10, y + 9, WHITE);
    display.drawLine(x + 2, y + 12, x + 8, y + 12, WHITE);
  } else if (cond.indexOf("霾") >= 0) {
    display.drawLine(x + 2, y + 6, x + 6, y + 6, WHITE);
    display.drawLine(x + 8, y + 6, x + 12, y + 6, WHITE);
    display.drawLine(x + 3, y + 9, x + 7, y + 9, WHITE);
    display.drawLine(x + 9, y + 9, x + 13, y + 9, WHITE);
    display.drawLine(x + 2, y + 12, x + 6, y + 12, WHITE);
    display.drawLine(x + 8, y + 12, x + 12, y + 12, WHITE);
  } else if (cond.indexOf("沙尘") >= 0 || cond.indexOf("沙") >= 0) {
    display.drawCircle(x + 4, y + 6, 3, WHITE);
    display.drawCircle(x + 8, y + 4, 4, WHITE);
    display.drawCircle(x + 12, y + 6, 3, WHITE);
    display.drawRect(x + 1, y + 6, 14, 4, WHITE);
    display.drawLine(x + 2, y + 11, x + 5, y + 11, WHITE);
    display.drawLine(x + 7, y + 13, x + 10, y + 13, WHITE);
    display.drawLine(x + 3, y + 15, x + 6, y + 15, WHITE);
  } else {
    display.drawCircle(x + 4, y + 6, 3, WHITE);
    display.drawCircle(x + 8, y + 4, 4, WHITE);
    display.drawCircle(x + 12, y + 6, 3, WHITE);
    display.drawRect(x + 1, y + 6, 14, 4, WHITE);
  }
}

//==================== 手动 gunzip ====================
String httpGetGunzip(HTTPClient& http) {
  WiFiClient* stream = http.getStreamPtr();
  if (!stream) return "";

  static uint8_t gzBuf[16384];
  size_t gzLen = 0;
  memset(gzBuf, 0, sizeof(gzBuf));

  unsigned long tLastData = millis();
  while (millis() - tLastData < 5000) {
    int avail = stream->available();
    if (avail > 0) {
      if (gzLen + avail > sizeof(gzBuf)) avail = sizeof(gzBuf) - gzLen;
      int n = stream->read(gzBuf + gzLen, avail);
      if (n <= 0) break;
      gzLen += n;
      tLastData = millis();
    } else {
      delay(10);
      if (!stream->connected() && stream->available() == 0) break;
    }
  }

  Serial.printf("gzip 数据总长: %d\n", gzLen);
  if (gzLen < 18) return "";
  Serial.printf("前 2 字节: %02X %02X\n", gzBuf[0], gzBuf[1]);

  if (gzBuf[0] != 0x1F || gzBuf[1] != 0x8B) {
    String raw;
    raw.reserve(gzLen);
    raw.concat((const char*)gzBuf, gzLen);
    return raw;
  }

  uint8_t* deflateData = gzBuf + 10;
  size_t deflateLen = gzLen - 18;

  static tinfl_decompressor decomp;
  memset(&decomp, 0, sizeof(decomp));
  tinfl_init(&decomp);

  static uint8_t outBuf[32768];

  size_t outLen = sizeof(outBuf);
  size_t inConsumed = 0;
  tinfl_status status = tinfl_decompress(
    &decomp,
    deflateData, &deflateLen,
    outBuf, outBuf, &outLen,
    0
  );

  Serial.printf("解压状态=%d, 输出长度=%d, inConsumed=%d\n",
                status, (int)outLen, (int)(gzLen - 18 - deflateLen));

  while (status == TINFL_STATUS_HAS_MORE_OUTPUT) {
    size_t moreOut = sizeof(outBuf) - outLen;
    size_t consumed2 = 0;
    status = tinfl_decompress(
      &decomp,
      deflateData + (gzLen - 18), &consumed2,
      outBuf + outLen, outBuf + outLen, &moreOut,
      0
    );
    outLen += moreOut;
    Serial.printf("继续解压 status=%d, 累计输出=%d\n", status, (int)outLen);
    if (moreOut == 0) break;
  }

  if (status != TINFL_STATUS_DONE && status != TINFL_STATUS_HAS_MORE_OUTPUT && status != TINFL_STATUS_NEEDS_MORE_INPUT) {
    Serial.printf("解压失败 status=%d\n", status);
    return "";
  }

  String result;
  result.reserve(outLen);
  result.concat((const char*)outBuf, outLen);
  return result;
}

//==================== 农历 API ====================
String getLunarFromAPI(int y, int m, int d) {
  if (WiFi.status() != WL_CONNECTED) return "";

  String url = String("https://cn.apihz.cn/api/time/getzdday.php?id=") + LUNAR_API_ID +
               "&key=" + LUNAR_API_KEY +
               "&nian=" + String(y) +
               "&yue=" + String(m) +
               "&ri=" + String(d);

  HTTPClient http;
  http.setTimeout(8000);
  http.begin(url);

  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("农历 API http 失败 code=%d\n", httpCode);
    http.end();
    return "";
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("农历 json 解析失败: %s\n", err.c_str());
    return "";
  }

  if (doc["code"].as<int>() != 200) {
    Serial.printf("农历 API 错误 code=%d\n", doc["code"].as<int>());
    return "";
  }

  String nyue = doc["nyue"].as<String>();
  String nri  = doc["nri"].as<String>();

  String result = nyue + nri;
  Serial.println("农历 API 返回: " + result);
  return result;
}

//==================== 天气 ====================
bool getForecast(String lat, String lng);

bool getCityAndWeather() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.setTimeout(8000);
  http.begin("http://ip9.com.cn/get");
  int httpCode = http.GET();
  if (httpCode <= 0) { Serial.printf("ip9 请求失败,code=%d\n", httpCode); http.end(); return false; }
  String payload = http.getString();
  Serial.println("ip9 返回:" + payload);
  http.end();

  StaticJsonDocument<1024> ipDoc;
  if (deserializeJson(ipDoc, payload)) { Serial.println("ip9 json解析失败"); return false; }
  const char* country = ipDoc["data"]["country"].as<const char*>();
  if (country == nullptr || String(country) != "中国") { Serial.println("ip9:不在国内"); return false; }

  String cityIp = ipDoc["data"]["city"].as<String>();
  String lngStr = ipDoc["data"]["lng"].as<String>();
  String latStr = ipDoc["data"]["lat"].as<String>();
  Serial.printf("定位：%s lat=%s lon=%s\n", cityIp.c_str(), latStr.c_str(), lngStr.c_str());

  String url = String("https://") + QWEATHER_API_HOST + "/weather/v1/current/" + latStr + "/" + lngStr;

  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  http.begin(secureClient, url);
  http.addHeader("X-QW-Api-Key", QWEATHER_API_KEY);

  httpCode = http.GET();
  if (httpCode <= 0) { Serial.printf("天气http失败 code=%d\n", httpCode); http.end(); return false; }

  payload = httpGetGunzip(http);
  if (payload.length() == 0) {
    Serial.println("gunzip 返回空");
    http.end();
    return false;
  }
  http.end();
  Serial.println("V1接口返回：" + payload);

  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, payload)) { Serial.println("天气json解析失败"); return false; }

  strncpy(cityName, cityIp.c_str(), sizeof(cityName) - 1);
  cityName[sizeof(cityName) - 1] = '\0';

  if (doc["condition"].is<JsonObject>() && doc["condition"]["text"].is<const char*>()) {
    strncpy(weatherInfo, doc["condition"]["text"].as<const char*>(), sizeof(weatherInfo) - 1);
    weatherInfo[sizeof(weatherInfo) - 1] = '\0';
  } else {
    strncpy(weatherInfo, "无数据", sizeof(weatherInfo) - 1);
  }

  if (doc["temperature"].is<JsonObject>() && doc["temperature"]["value"].is<float>()) {
    temp = doc["temperature"]["value"].as<float>();
  } else {
    temp = 0;
  }

  if (doc["humidity"].is<float>() || doc["humidity"].is<int>()) {
    float h = doc["humidity"].as<float>();
    humidity = (h <= 1.0f) ? (int)(h * 100.0f) : (int)h;
  } else {
    humidity = 0;
  }

  offlineMode = false;
  prefs.putString("city", String(cityName));
  prefs.putString("weatherText", String(weatherInfo));
  prefs.putFloat("temp", temp);
  prefs.putInt("hum", humidity);

  if (!getForecast(latStr, lngStr)) Serial.println("预报更新失败");

  Serial.println("====天气更新成功====");
  return true;
}

void loadWeatherCache() {
  String tmpCity = prefs.getString("city", "未知");
  String tmpWea = prefs.getString("weatherText", "无数据");
  strncpy(cityName, tmpCity.c_str(), sizeof(cityName) - 1);
  strncpy(weatherInfo, tmpWea.c_str(), sizeof(weatherInfo) - 1);
  temp = prefs.getFloat("temp", 20);
  humidity = prefs.getInt("hum", 50);
}

bool getForecast(String lat, String lng) {
  if (WiFi.status() != WL_CONNECTED) return false;

  String url = String("https://") + QWEATHER_API_HOST + "/weather/v1/daily/" + lat + "/" + lng + "?numdays=7";

  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPClient http;
  http.setTimeout(8000);
  http.begin(secureClient, url);
  http.addHeader("X-QW-Api-Key", QWEATHER_API_KEY);

  int httpCode = http.GET();
  if (httpCode <= 0) { Serial.printf("预报http失败 code=%d\n", httpCode); http.end(); return false; }

  String payload = httpGetGunzip(http);
  if (payload.length() == 0) {
    Serial.println("预报 gunzip 返回空");
    http.end();
    return false;
  }
  http.end();
  Serial.println("预报返回:" + payload);

  DynamicJsonDocument filter(1024);
  {
    JsonObject filterRoot = filter.to<JsonObject>();
    JsonArray daysFilter = filterRoot.createNestedArray("days");
    JsonObject dayFilter = daysFilter.createNestedObject();
    dayFilter["forecastStartTime"] = true;
    JsonObject dayCond = dayFilter.createNestedObject("daytime");
    JsonObject dayCond2 = dayCond.createNestedObject("condition");
    dayCond2["text"] = true;
    JsonObject tMax = dayFilter.createNestedObject("temperatureMax");
    tMax["value"] = true;
    JsonObject tMin = dayFilter.createNestedObject("temperatureMin");
    tMin["value"] = true;
  }

  DynamicJsonDocument doc(4096);
  DeserializationError err2 = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
  if (err2) {
    Serial.printf("预报json解析失败: %s\n", err2.c_str());
    return false;
  }

  JsonArray days = doc["days"].as<JsonArray>();
  forecastCount = 0;

  for (JsonObject day : days) {
    if (forecastCount >= 7) break;
    ForecastDay* f = &forecast[forecastCount];

    String dateStr = day["forecastStartTime"].as<String>();
    int firstDash = dateStr.indexOf('-');
    int secondDash = dateStr.indexOf('-', firstDash + 1);
    if (firstDash > 0 && secondDash > firstDash) {
      f->month = dateStr.substring(firstDash + 1, secondDash).toInt();
      int tPos = dateStr.indexOf('T');
      if (tPos > secondDash) f->day = dateStr.substring(secondDash + 1, tPos).toInt();
      else f->day = dateStr.substring(secondDash + 1).toInt();
    } else { f->month = 0; f->day = 0; }

    if (day["daytime"].is<JsonObject>() && day["daytime"]["condition"]["text"].is<const char*>()) {
      strncpy(f->text, day["daytime"]["condition"]["text"].as<const char*>(), sizeof(f->text) - 1);
      f->text[sizeof(f->text) - 1] = '\0';
    } else {
      strcpy(f->text, "未知");
    }

    if (day["temperatureMax"].is<JsonObject>()) f->tempMax = day["temperatureMax"]["value"].as<float>();
    else f->tempMax = 0;

    if (day["temperatureMin"].is<JsonObject>()) f->tempMin = day["temperatureMin"]["value"].as<float>();
    else f->tempMin = 0;

    forecastCount++;
  }

  forecastLoaded = true;
  Serial.printf("====预报更新成功 %d天====\n", forecastCount);
  return true;
}

//==================== WiFi扫描 ====================
void scanWiFi() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Scanning WiFi...");
  display.display();
  int nRaw = WiFi.scanNetworks();
  wifiScanCount = constrain(nRaw, 0, WIFI_SCAN_MAX);
  for (int i = 0; i < WIFI_SCAN_MAX; i++) wifiList[i] = "";
  for (int i = 0; i < wifiScanCount; i++) wifiList[i] = WiFi.SSID(i);
  WiFi.scanDelete();
}

bool connectWiFi(String ssid, String pass) {
  prefs.putString("ssid", ssid);
  saveWiFiCfg(ssid, pass);
  WiFi.disconnect();
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long start = millis();
  const unsigned long WIFI_TIMEOUT = 8000;
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT) beepTask();
  showWifiTip = true;
  wifiTipStart = millis();
  return (WiFi.status() == WL_CONNECTED);
}

//==================== 页面绘制 ====================
void drawClockPage() {
  display.clearDisplay();
  time_t utcEpoch = timeClient.getEpochTime();
  struct tm tmBuf;
  gmtime_r(&utcEpoch, &tmBuf);

  year  = tmBuf.tm_year + 1900;
  mon   = tmBuf.tm_mon + 1;
  mday  = tmBuf.tm_mday;
  hourNow = timeClient.getHours();
  minNow  = timeClient.getMinutes();
  secNow  = timeClient.getSeconds();

  // ★ 农历缓存：每天只请求一次 API
  if (cachedLunarYear != year || cachedLunarMon != mon || cachedLunarDay != mday || cachedLunar.length() == 0) {
    String apiResult = getLunarFromAPI(year, mon, mday);
    if (apiResult.length() > 0) {
      cachedLunar = apiResult;
      cachedLunarYear = year;
      cachedLunarMon = mon;
      cachedLunarDay = mday;
    } else {
      if (cachedLunar.length() == 0) cachedLunar = "农历";
    }
  }
  String lunarStr = cachedLunar;

  const char *weekStr[] = {"星期日","星期一","星期二","星期三","星期四","星期五","星期六"};
  String weekStrStr = weekStr[tmBuf.tm_wday];

  drawWifiIcon(WiFi.RSSI());

  // ===== 大号时间（带秒，24px 字体，居中）=====
  char timeBuf[12];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", hourNow, minNow, secNow);

  u8g2.setFont(u8g2_font_logisoso24_tn);
  u8g2.setForegroundColor(SSD1306_WHITE);
  u8g2.setBackgroundColor(SSD1306_BLACK);
  int tw = u8g2.getUTF8Width(timeBuf);
  int tx = (SCREEN_WIDTH - tw) / 2;
  u8g2.setCursor(tx, 36);
  u8g2.print(timeBuf);

  // ===== 日期行 =====
  String dateLine = String(mon) + "/" + String(mday) + " " + weekStrStr;
  if (offlineMode) dateLine += " 离线";
  drawCN(0, 50, dateLine);

  // ===== 农历行 =====
  drawCN(0, 64, lunarStr);

  display.display();
}

void drawWeatherPage() {
  display.clearDisplay();
  drawWifiIcon(WiFi.RSSI());
  drawWeatherIcon(44, 2, String(weatherInfo));

  String line1 = String(cityName) + " " + String(weatherInfo) + " " + String(temp, 1) + "℃ " + String(humidity) + "%";
  drawCN(0, 26, line1);
  display.drawLine(0, 28, 127, 28, WHITE);

  if (forecastLoaded && forecastCount > 0) {
    for (int i = 0; i < forecastCount && i < 7; i++) drawWeatherIcon(i * 18, 30, String(forecast[i].text));
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    for (int i = 0; i < forecastCount && i < 7; i++) {
      display.setCursor(i * 18 + 4, 48);
      display.print(forecast[i].day);
    }
  } else {
    drawCN(0, 44, "等待预报数据...");
  }

  if (offlineMode) drawCN(0, 62, "离线");
  display.display();
}

void drawMenu() {
  display.clearDisplay();
  drawCN(0, 14, "====菜单====");
  drawCN(0, 32, menuSelection == 0 ? "> WiFi设置" : "  WiFi设置");
  drawCN(0, 50, menuSelection == 1 ? "> 返回" : "  返回");
  display.display();
}

void drawWifiPassInput() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("WiFi:");
  display.print(selectedSSID);
  display.setCursor(0, 12);
  display.print("PASS:");
  display.print(inputPASS);
  display.print(charSet[charIndex]);
  drawCN(0, 50, "上下选字符 右添加");
  drawCN(0, 62, "左退格");
  display.display();
}

void drawWifiScanList() {
  display.clearDisplay();
  drawCN(0, 14, "WiFi列表:");
  if (wifiScanCount == 0) {
    drawCN(0, 32, "无WiFi");
    drawCN(0, 62, "左退出 右选择");
    display.display();
    return;
  }
  const int maxShow = 4;
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < maxShow; i++) {
    int idx = startLine + i;
    if (idx >= wifiScanCount) break;
    display.setCursor(0, 24 + (12 * i));
    display.print(idx == wifiSelectIdx ? ">" : " ");
    display.print(wifiList[idx]);
  }
  drawCN(0, 62, "左退出 右选择");
  display.display();
}

void drawTipScreen(String msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println(msg);
  display.display();
}

void drawWifiSetupPage() {
  display.clearDisplay();
  drawCN(0, 12, "WiFi设置");
  drawWifiIcon(WiFi.RSSI());

  String savedSSID = prefs.getString("ssid", "");
  String savedPass = loadWiFiPass();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.print("SSID:");
  display.print(savedSSID.length() > 0 ? savedSSID : "<none>");
  display.setCursor(0, 28);
  display.print("PASS:");
  if (savedPass.length() > 0) {
    for (size_t i = 0; i < savedPass.length(); i++) display.print("*");
  } else display.print("<none>");

  drawCN(0, 42, wifiSetupSelection == 0 ? "> 连接WiFi" : "  连接WiFi");
  drawCN(0, 54, wifiSetupSelection == 1 ? "> 修改WiFi" : "  修改WiFi");
  drawCN(0, 64, wifiSetupSelection == 2 ? "> 返回" : "  返回");
  display.display();
}

//==================== 按键检测 ====================
bool readKey(uint8_t pin) { return digitalRead(pin) == LOW; }
uint8_t keyScanLoop() {
  static unsigned long keyPressStart = 0;
  static uint8_t lastKey = 0;
  uint8_t pressed = 0;
  if (readKey(KEY_UP)) pressed = KEY_UP;
  if (readKey(KEY_DOWN)) pressed = KEY_DOWN;
  if (readKey(KEY_LEFT)) pressed = KEY_LEFT;
  if (readKey(KEY_RIGHT)) pressed = KEY_RIGHT;

  if (pressed != 0 && lastKey == 0) {
    keyPressStart = millis(); lastKey = pressed;
    screenActiveTimer = millis(); screenOn = true;
  }
  if (pressed == 0 && lastKey != 0) {
    unsigned long dur = millis() - keyPressStart;
    uint8_t ret = lastKey;
    lastKey = 0;
    screenActiveTimer = millis(); screenOn = true;
    startShortBeep();
    if (dur > 2000) return ret | 0x80;
    return ret;
  }
  return 0;
}

void handleKey(uint8_t k) {
  bool longPress = k & 0x80;
  uint8_t key = k & 0x7F;

  if (currentPage == PAGE_CLOCK || currentPage == PAGE_WEATHER) {
    if (longPress && (key == KEY_LEFT || key == KEY_RIGHT)) { hourBellEnable = !hourBellEnable; return; }
    if (!longPress) {
      if (key == KEY_LEFT) setPage(PAGE_CLOCK);
      if (key == KEY_RIGHT) setPage(PAGE_WEATHER);
      if (key == KEY_UP) setPage(PAGE_MENU);
    }
  } else if (currentPage == PAGE_MENU) {
    if (!longPress) {
      if (key == KEY_UP) menuSelection = 0;
      if (key == KEY_DOWN) menuSelection = 1;
      if (key == KEY_RIGHT) {
        if (menuSelection == 0) { wifiSetupSelection = 0; setPage(PAGE_WIFI_SETUP); }
        else if (menuSelection == 1) setPage(PAGE_CLOCK);
      }
      if (key == KEY_LEFT) setPage(PAGE_CLOCK);
    }
  } else if (currentPage == PAGE_WIFI_SCAN) {
    if (!longPress) {
      if (key == KEY_UP) {
        wifiSelectIdx--;
        if (wifiSelectIdx < 0) wifiSelectIdx = wifiScanCount - 1;
        if (wifiSelectIdx < startLine) startLine = wifiSelectIdx;
      }
      if (key == KEY_DOWN) {
        wifiSelectIdx++;
        if (wifiSelectIdx >= wifiScanCount) wifiSelectIdx = 0;
        if (wifiSelectIdx >= startLine + maxShow) startLine = wifiSelectIdx - maxShow + 1;
      }
      if (key == KEY_RIGHT) {
        selectedSSID = wifiList[wifiSelectIdx];
        inputPASS = ""; charIndex = 0;
        setPage(PAGE_WIFI_PASS);
      }
      if (key == KEY_LEFT) setPage(PAGE_MENU);
    }
  } else if (currentPage == PAGE_WIFI_PASS) {
    if (!longPress) {
      if (key == KEY_UP) { charIndex++; if (charIndex >= sizeof(charSet) - 1) charIndex = 0; }
      if (key == KEY_DOWN) { charIndex--; if (charIndex < 0) charIndex = sizeof(charSet) - 2; }
      if (key == KEY_RIGHT) { if (inputPASS.length() < WIFI_PASS_MAXLEN) inputPASS += charSet[charIndex]; }
      if (key == KEY_LEFT) { if (inputPASS.length() > 0) inputPASS.remove(inputPASS.length() - 1); }
    }
    if (longPress) {
      wifiManualConfig = true;
      bool ok = connectWiFi(selectedSSID, inputPASS);
      if (ok) { setPage(PAGE_CLOCK); lastNtpUpdate = 0; lastWeatherUpdate = 0; }
    }
  } else if (currentPage == PAGE_WIFI_SETUP) {
    if (!longPress) {
      if (key == KEY_UP) { wifiSetupSelection--; if (wifiSetupSelection < 0) wifiSetupSelection = 2; }
      if (key == KEY_DOWN) { wifiSetupSelection++; if (wifiSetupSelection > 2) wifiSetupSelection = 0; }
      if (key == KEY_RIGHT) {
        if (wifiSetupSelection == 0) {
          String savedSSID = prefs.getString("ssid", "");
          String savedPass = loadWiFiPass();
          if (savedSSID.length() > 0 && savedPass.length() > 0) {
            wifiManualConfig = true;
            bool ok = connectWiFi(savedSSID, savedPass);
            if (ok) { setPage(PAGE_CLOCK); lastNtpUpdate = 0; lastWeatherUpdate = 0; }
          } else {
            scanWiFi(); wifiSelectIdx = 0; startLine = 0;
            setPage(PAGE_WIFI_SCAN);
          }
        } else if (wifiSetupSelection == 1) {
          wifiManualConfig = true;
          scanWiFi(); wifiSelectIdx = 0; startLine = 0;
          setPage(PAGE_WIFI_SCAN);
        } else if (wifiSetupSelection == 2) setPage(PAGE_CLOCK);
      }
      if (key == KEY_LEFT) setPage(PAGE_MENU);
    }
  }
}

//==================== 开机启动画面 ====================
void drawBootScreen(int progress, const char* subMsg = "") {
  display.clearDisplay();

  // 标题
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setForegroundColor(SSD1306_WHITE);
  u8g2.setBackgroundColor(SSD1306_BLACK);
  u8g2.setCursor(28, 16);
  u8g2.print("正在启动...");

  // 进度条边框
  display.drawRect(14, 30, 100, 10, WHITE);
  // 进度条填充
  int fillW = (96 * progress) / 100;
  if (fillW > 0) display.fillRect(16, 32, fillW, 6, WHITE);

  // 副标题
  if (subMsg != nullptr && subMsg[0] != '\0') {
    u8g2.setCursor(0, 56);
    u8g2.print(subMsg);
  } else {
    u8g2.setCursor(20, 56);
    u8g2.print("SmartClock v25");
  }

  display.display();
}

//==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== ESP32 Startup ===");
  Serial.println("Version: 第二十五版(启动时连WiFi+同步时间)");

  delay(500);

  pinMode(BEEP_PIN, OUTPUT);
  pinMode(KEY_UP, INPUT_PULLUP);
  pinMode(KEY_DOWN, INPUT_PULLUP);
  pinMode(KEY_LEFT, INPUT_PULLUP);
  pinMode(KEY_RIGHT, INPUT_PULLUP);

  delay(200);
  Wire.begin(OLED_SDA, OLED_SCL);
  delay(200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println(F("OLED init fail"));
  display.clearDisplay();
  display.display();
  delay(50);

  u8g2.begin(display);
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setForegroundColor(SSD1306_WHITE);
  u8g2.setBackgroundColor(SSD1306_BLACK);
  Serial.println("OLED + u8g2 ready");

  prefs.begin("wificfg");
  delay(100);

  loadWeatherCache();
  String savedSSID = prefs.getString("ssid", "");
  String savedPass = loadWiFiPass();

  // ===== 阶段 1：0% → 30%，初始化 =====
  for (int p = 0; p <= 30; p += 5) {
    drawBootScreen(p, "初始化...");
    delay(60);
  }

  // ★ 在动画中发起 WiFi 连接（非阻塞）
  bool wifiStarted = false;
  if (savedSSID != "" && savedPass != "") {
    Serial.printf("正在连接已保存的WiFi: %s\n", savedSSID.c_str());
    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
    wifiStarted = true;
  } else {
    Serial.println("未保存WiFi配置，跳过连接");
  }

  // ===== 阶段 2：30% → 70%，配置 NTP =====
  configTime(8 * 3600, 0, "ntp1.aliyun.com", "ntp.ntsc.ac.cn");
  Serial.println("configTime 已设置");

  for (int p = 35; p <= 70; p += 5) {
    drawBootScreen(p, wifiStarted ? "连接 WiFi..." : "跳过 WiFi...");
    delay(80);
    beepTask();
  }

  // ===== 阶段 3：等待 WiFi 连接（最多 8 秒）=====
  if (wifiStarted) {
    unsigned long wifiStart = millis();
    const unsigned long WIFI_BOOT_TIMEOUT = 8000;
    int dotCnt = 0;
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < WIFI_BOOT_TIMEOUT) {
      dotCnt = (dotCnt + 1) % 4;
      char msg[40];
      snprintf(msg, sizeof(msg), "连接 WiFi%.*s", dotCnt, "...");
      drawBootScreen(70, msg);
      delay(250);
      beepTask();
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("WiFi 已连接，IP=%s\n", WiFi.localIP().toString().c_str());
      drawBootScreen(75, "WiFi 已连接");
    } else {
      Serial.println("WiFi 连接超时");
      drawBootScreen(75, "WiFi 超时，离线模式");
      offlineMode = true;
    }
    delay(300);
  }

  // ===== 阶段 4：75% → 90%，等待 NTP 同步（最多 5 秒）=====
  for (int p = 75; p <= 90; p += 5) {
    drawBootScreen(p, "同步时间...");
    delay(60);
    beepTask();
  }

  if (WiFi.status() == WL_CONNECTED) {
    unsigned long ntpStart = millis();
    const unsigned long NTP_BOOT_TIMEOUT = 5000;
    int dotCnt = 0;
    bool synced = false;
    while (millis() - ntpStart < NTP_BOOT_TIMEOUT) {
      dotCnt = (dotCnt + 1) % 4;
      char msg[40];
      snprintf(msg, sizeof(msg), "同步时间%.*s", dotCnt, "...");
      drawBootScreen(90, msg);
      delay(250);
      beepTask();

      if (timeClient.forceUpdate()) {
        lastNtpUpdate = millis();
        timeSynced = true;
        synced = true;
        Serial.println("NTP 同步成功");
        break;
      }
    }
    if (!synced) {
      Serial.println("NTP 同步超时，使用默认时间");
      drawBootScreen(90, "时间同步超时");
      delay(400);
    }
  } else {
    drawBootScreen(90, "离线模式");
    delay(400);
  }

  // ===== 阶段 5：90% → 100%，收尾 =====
  for (int p = 95; p <= 100; p += 5) {
    drawBootScreen(p, "就绪");
    delay(80);
  }

  drawBootScreen(100, "启动完成");
  delay(400);

  Serial.println("=== Setup Done ===");
  screenActiveTimer = millis();

  // 首次进入主界面时触发天气更新
  lastWeatherUpdate = 0;
}

void loop() {
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 30000) {
    lastHeartbeat = millis();
    Serial.printf("[heartbeat] Running... WiFi=%d temp=%.1f page=%d screenOn=%d\n",
                  WiFi.status() == WL_CONNECTED ? 1 : 0, temp, currentPage, screenOn);
  }

  unsigned long now = millis();
  uint8_t key = keyScanLoop();
  if (key != 0) { handleKey(key); screenActiveTimer = millis(); screenOn = true; }
  beepTask();

  if (!wifiManualConfig && WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiReconnectTry > WIFI_RECONNECT_INTERVAL) {
      lastWifiReconnectTry = now;
      String savedSSID = prefs.getString("ssid", "");
      String savedPass = loadWiFiPass();
      if (savedSSID.length() > 0 && savedPass.length() > 0) WiFi.begin(savedSSID.c_str(), savedPass.c_str());
    }
  }
  if (WiFi.status() == WL_CONNECTED) wifiManualConfig = false;

  if (showWifiTip) {
    if (millis() - wifiTipStart < WIFI_TIP_DURATION) {
      drawTipScreen(WiFi.status() == WL_CONNECTED ? "WiFi OK" : "WiFi FAIL");
    } else showWifiTip = false;
  }

  bool isMainPage = (currentPage == PAGE_CLOCK || currentPage == PAGE_WEATHER);
  bool isNight = timeSynced && (hourNow >= 21 || hourNow < 7);

  if (isMainPage && isNight) {
    if (millis() - screenActiveTimer > NIGHT_SCREEN_TIMEOUT) screenOn = false;
  } else {
    screenOn = true;
    screenActiveTimer = millis();
  }

  if (WiFi.status() == WL_CONNECTED && millis() - lastNtpUpdate > NTP_INTERVAL) {
    if (timeClient.update()) { lastNtpUpdate = now; offlineMode = false; timeSynced = true; }
  } else if (WiFi.status() != WL_CONNECTED) offlineMode = true;

  static bool firstWeatherTried = false;
  if (WiFi.status() == WL_CONNECTED &&
      (!firstWeatherTried || millis() - lastWeatherUpdate > WEATHER_INTERVAL)) {
    firstWeatherTried = true;
    if (getCityAndWeather()) lastWeatherUpdate = now;
  }

  int currentMin = timeClient.getMinutes();
  if (hourBellEnable && currentMin == 0 && lastMin != 0) startHourBeep(timeClient.getHours());
  lastMin = currentMin;

  if (screenOn && !showWifiTip) {
    display.ssd1306_command(SSD1306_DISPLAYON);
    delay(1);
    switch (currentPage) {
      case PAGE_CLOCK:      drawClockPage();     break;
      case PAGE_WEATHER:    drawWeatherPage();   break;
      case PAGE_MENU:       drawMenu();          break;
      case PAGE_WIFI_SCAN:  drawWifiScanList();  break;
      case PAGE_WIFI_PASS:  drawWifiPassInput(); break;
      case PAGE_WIFI_SETUP: drawWifiSetupPage(); break;
    }
  } else if (!screenOn) display.ssd1306_command(SSD1306_DISPLAYOFF);
}