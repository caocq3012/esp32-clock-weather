#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//===================== 硬件配置 =====================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define BEEP_PIN       23
#define KEY_UP         27
#define KEY_DOWN       26
#define KEY_LEFT       25
#define KEY_RIGHT      33
#define OLED_SDA       19
#define OLED_SCL       18
//====================================================

#define DEBOUNCE_MS  50

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const uint8_t KEYS[4]     = { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT };
const char*   KEY_NAME[4] = { "UP", "DOWN", "LEFT", "RIGHT" };

bool lastState[4]  = { HIGH, HIGH, HIGH, HIGH };
bool lastStable[4] = { HIGH, HIGH, HIGH, HIGH };
unsigned long lastChange[4] = { 0, 0, 0, 0 };
unsigned long pressCount[4] = { 0, 0, 0, 0 };

String lastKey = "-";
unsigned long lastKeyTotal = 0;

void render() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("=== KEY TEST ==="));

  display.setTextSize(2);
  display.setCursor(0, 14);
  display.print("KEY: ");
  display.println(lastKey);

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("Total: "); display.println(lastKeyTotal);
  display.setCursor(0, 52);
  display.print("U:"); display.print(pressCount[0]);
  display.print(" D:"); display.print(pressCount[1]);
  display.print(" L:"); display.print(pressCount[2]);
  display.print(" R:"); display.print(pressCount[3]);

  display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);          // I2C 用你的自定义引脚 19/18

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // 不亮就把 0x3C 换 0x3D
    Serial.println(F("SSD1306 not found!"));
    for (;;) delay(100);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("OLED OK"));
  display.println(F("Press any key"));
  display.display();

  for (int i = 0; i < 4; i++) pinMode(KEYS[i], INPUT_PULLUP);  // 按下=低电平

  pinMode(BEEP_PIN, OUTPUT);
  digitalWrite(BEEP_PIN, LOW);             // 上电静音，防止“滴”一声

  delay(300);
  render();
}

void loop() {
  bool changed = false;
  for (int i = 0; i < 4; i++) {
    bool raw = digitalRead(KEYS[i]);
    unsigned long now = millis();

    if (raw != lastState[i]) {            // 电平跳变，记消抖起点
      lastState[i] = raw;
      lastChange[i] = now;
    }
    if ((now - lastChange[i]) >= DEBOUNCE_MS && raw != lastStable[i]) {
      lastStable[i] = raw;
      if (lastStable[i] == LOW) {         // 稳定后的高→低 = 按下
        pressCount[i]++;
        lastKeyTotal++;
        lastKey = KEY_NAME[i];
        changed = true;
        Serial.printf("Key: %s count=%lu\n", KEY_NAME[i], pressCount[i]);
      }
    }
  }
  if (changed) render();                   // 只在有按键时刷屏
}