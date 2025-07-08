#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define LED_PIN 8
#define LED_WIDTH 16
#define LED_HEIGHT 8
#define NUM_LEDS (LED_WIDTH * LED_HEIGHT)

Adafruit_NeoPixel leds(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t (*fontData)[8] = nullptr;  // 動的に読み込んだフォントデータ
int numChars = 0;                  // フォントの文字数

uint8_t brightness = 50;

//littlefsのファイルパス
const char *FONT_JSON_PATH = "/font.json";

// LittleFS 上にフォントファイルが存在しなければ、デフォルトのオブジェクトをJSONとして作成する
void createDefaultFontFile() {
  StaticJsonDocument<512> doc;
  JsonObject root = doc.to<JsonObject>();

  // デフォルト明るさ値を設定
  root["brightness"] = brightness;
  // フォント配列を作成
  JsonArray arr = root.createNestedArray("font");

  JsonArray char0 = arr.createNestedArray();
  uint8_t default0[8] = {
    0b10101100,
    0b01110110,
    0b10101010,
    0b01001010,
    0b11111010,
    0b01010100,
    0b11101010,
    0b00000000
  };
  for (int i = 0; i < 8; i++) char0.add(default0[i]);

  JsonArray char1 = arr.createNestedArray();
  uint8_t default1[8] = {
    0b11111110,
    0b01001010,
    0b01001010,
    0b10011110,
    0b01101010,
    0b01101010,
    0b00010010,
    0b00000000
  };
  for (int i = 0; i < 8; i++) char1.add(default1[i]);

  File file = LittleFS.open(FONT_JSON_PATH, "w");
  if (!file) {
    Serial.println("フォント JSON ファイルの作成に失敗");
    return;
  }
  serializeJson(doc, file);
  file.close();
}

// LittleFS上のSONを読み込む
bool loadFontFromFS() {
  if (fontData != nullptr) {
    free(fontData);
    fontData = nullptr;
    numChars = 0;
  }

  File file = LittleFS.open(FONT_JSON_PATH, "r");
  if (!file) {
    Serial.println("フォントファイルが見つかりません");
    return false;
  }
  size_t size = file.size();
  std::unique_ptr<char[]> buf(new char[size + 1]);
  file.readBytes(buf.get(), size);
  buf.get()[size] = '\0';
  file.close();

  DynamicJsonDocument doc(2048 + size);
  auto error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return false;
  }

  JsonObject root = doc.as<JsonObject>();

  if (root.containsKey("brightness")) {
    int tmp = root["brightness"].as<int>();
    if (tmp >= 0 && tmp <= 255) {
      brightness = (uint8_t)tmp;
    } else {
      Serial.println("brightness の値が範囲外です（0-255）。デフォルト値を使用します。");
    }
  } else {
    Serial.println("brightness フィールドが見つかりません。デフォルト値を使用します。");
  }

  if (!root.containsKey("font")) {
    Serial.println("font フィールドが見つかりません");
    return false;
  }
  JsonArray arr = root["font"].as<JsonArray>();
  int count = arr.size();
  if (count <= 0) {
    Serial.println("フォントデータが空です");
    return false;
  }

  // 動的メモリを確保
  fontData = (uint8_t(*)[8])malloc(sizeof(uint8_t[8]) * count);
  if (!fontData) {
    Serial.println("メモリ確保失敗");
    return false;
  }

  // JSON 配列を読み込んで fontData に格納
  for (int i = 0; i < count; i++) {
    JsonArray row = arr[i].as<JsonArray>();
    if (row.size() != 8) {
      Serial.println("各文字のバイト数が 8 ではありません");
      free(fontData);
      fontData = nullptr;
      return false;
    }
    for (int j = 0; j < 8; j++) {
      fontData[i][j] = (uint8_t)row[j].as<int>();
    }
  }
  numChars = count;
  return true;
}

// シリアルから新しい JSON データを受信したら LittleFS に上書きして再読み込み
void checkSerialAndUpdateFont() {
  if (Serial.available()) {
    String jsonStr = Serial.readStringUntil('\n');
    jsonStr.trim();
    if (jsonStr.length() == 0) return;

    DynamicJsonDocument tmpDoc(2048);
    auto err = deserializeJson(tmpDoc, jsonStr);
    if (err) {
      Serial.print("受信 JSON パースエラー: ");
      Serial.println(err.c_str());
      return;
    }
    JsonObject root = tmpDoc.as<JsonObject>();

    if (root.containsKey("brightness")) {
      int tmpB = root["brightness"].as<int>();
      if (tmpB >= 0 && tmpB <= 255) {
        brightness = (uint8_t)tmpB;
        leds.setBrightness(brightness);
        Serial.print("brightness を更新しました: ");
        Serial.println(brightness);
      } else {
        Serial.println("受信 brightness の値が範囲外（0-255）です");
      }
    }

    File file = LittleFS.open(FONT_JSON_PATH, "w");
    if (!file) {
      Serial.println("フォントファイル書き込み失敗");
      return;
    }
    file.print(jsonStr);
    file.close();

    if (loadFontFromFS()) {
      Serial.println("フォントと明るさを更新しました。");
      leds.setBrightness(brightness);
    } else {
      Serial.println("フォント更新後の読み込みに失敗しました。");
    }
  }
}

// フェード倍率 0.80 → 20%ずつ薄れる
const float FADE_FACTOR = 0.80;

void hsv2rgb(float h, float s, float v, uint8_t &outR, uint8_t &outG, uint8_t &outB) {
  float c = v * s;
  float hh = h / 60.0;
  float x = c * (1 - fabs(fmod(hh, 2) - 1));
  float m = v - c;
  float r1, g1, b1;
  if (hh < 1) {
    r1 = c;
    g1 = x;
    b1 = 0;
  } else if (hh < 2) {
    r1 = x;
    g1 = c;
    b1 = 0;
  } else if (hh < 3) {
    r1 = 0;
    g1 = c;
    b1 = x;
  } else if (hh < 4) {
    r1 = 0;
    g1 = x;
    b1 = c;
  } else if (hh < 5) {
    r1 = x;
    g1 = 0;
    b1 = c;
  } else {
    r1 = c;
    g1 = 0;
    b1 = x;
  }
  outR = (uint8_t)((r1 + m) * 255);
  outG = (uint8_t)((g1 + m) * 255);
  outB = (uint8_t)((b1 + m) * 255);
}

// eoPixel のインデックス変換
int xyToIndex(int x, int y) {
  int invX = (LED_WIDTH - 1) - x;
  return y * LED_WIDTH + invX;
}

// フェード処理
void fadeAllPixels() {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint32_t prev = leds.getPixelColor(i);
    uint8_t r = (prev >> 16) & 0xFF;
    uint8_t g = (prev >> 8) & 0xFF;
    uint8_t b = (prev)&0xFF;
    r = (uint8_t)(r * FADE_FACTOR);
    g = (uint8_t)(g * FADE_FACTOR);
    b = (uint8_t)(b * FADE_FACTOR);
    leds.setPixelColor(i, r, g, b);
  }
}

// 文字を１つ描く関数
void drawCharDynamicColor(int charIndex, int xOffset, unsigned long timeBase) {
  if (charIndex < 0 || charIndex >= numChars) return;
  for (int y = 0; y < 8; y++) {
    uint8_t rowBits = fontData[charIndex][y];
    for (int x = 0; x < 8; x++) {
      int px = xOffset + x;
      if (px < 0 || px >= LED_WIDTH) continue;
      if (rowBits & (1 << (7 - x))) {
        float h = fmod((timeBase * 0.03f) + (px * 10) + (y * 5), 360.0f);
        float s = 0.80f;
        float v = 1.00f;
        uint8_t r, g, b;
        hsv2rgb(h, s, v, r, g, b);
        leds.setPixelColor(xyToIndex(px, y), r, g, b);
      }
    }
  }
}

static int scrollPos = 0;
static bool didStartPause = false;
static unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(9600);
  //while (!Serial) { delay(10); }

  Serial.println("1: setup 開始");

  if (!LittleFS.begin()) {
    Serial.println("2: LittleFS のマウントに失敗しました");
  } else {
    Serial.println("2: LittleFS マウント成功");
  }

  if (!LittleFS.exists(FONT_JSON_PATH)) {
    Serial.println("3: font.json が存在しないので新規作成します");
    createDefaultFontFile();
  } else {
    Serial.println("3: font.json が既に存在します");
  }

  if (loadFontFromFS()) {
    Serial.print("4: フォントを読み込みました。文字数＝");
    Serial.println(numChars);
  } else {
    Serial.println("4: フォント読み込みに失敗しました");
  }

  leds.begin();
  leds.clear();
  leds.setBrightness(brightness);
  leds.show();

  Serial.println("5: setup 完了");
}


void loop() {
  unsigned long now = millis();

  // ・シリアル受信時の処理
  checkSerialAndUpdateFont();

  fadeAllPixels();

  if (numChars == 1) {
    int xCenter = (LED_WIDTH - 8) / 2;
    drawCharDynamicColor(0, xCenter, now);
    leds.show();
    delay(100);
    return;
  } else if (numChars == 2) {
    drawCharDynamicColor(0, 0, now);
    drawCharDynamicColor(1, 8, now);
    leds.show();
    delay(100);
    return;
  } else {
    //スクロール処理
    int totalWidth = numChars * 8;
    int pausePos = 8 * (numChars - 2);

    if (!didStartPause && scrollPos == 0) {
      if (now - lastUpdate < 1000) {
        return;
      } else {
        didStartPause = true;
        scrollPos = 1;
        lastUpdate = now;
      }
    } else if (scrollPos > 0 && scrollPos < pausePos) {
      if (now - lastUpdate < 200) {
        return;
      } else {
        scrollPos++;
        lastUpdate = now;
      }
    } else if (scrollPos == pausePos) {
      if (now - lastUpdate < 1000) {
        return;
      } else {
        scrollPos = 0;
        didStartPause = false;
        lastUpdate = now;
      }
    }

    for (int i = 0; i < numChars; i++) {
      int xOffset = i * 8 - scrollPos;
      drawCharDynamicColor(i, xOffset, now);
    }
    leds.show();
    return;
  }
}
