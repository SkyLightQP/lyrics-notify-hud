#include <BluetoothSerial.h>
#include <U8g2lib.h>
#include <Wire.h>

#define TCA9548A_ADDRESS 0x70
#define OLED_WIDTH 128
#define OLED_HEIGHT 128
#define LINE_HEIGHT 20

U8G2_SH1107_SEEED_128X128_F_HW_I2C oled1(U8G2_R0, U8X8_PIN_NONE);
U8G2_SH1107_SEEED_128X128_F_HW_I2C oled2(U8G2_R0, U8X8_PIN_NONE);
U8G2_SH1107_SEEED_128X128_F_HW_I2C oled3(U8G2_R0, U8X8_PIN_NONE);
BluetoothSerial SerialBT;

char receivedData[128];
int dataIndex = 0;

void setup() {
    Wire.begin(21, 22);
    Serial.begin(115200);
    SerialBT.begin("LyricsNotify");

    delay(100);

    tcaSelect(0);
    oled1.begin();
    oled1.enableUTF8Print();
    oled1.setFont(u8g2_font_unifont_t_korean2);
    oled1.setFontDirection(0);
    oled1.setFontMode(1);
    oled1.clearBuffer();
    oled1.setCursor(0, 16);
    oled1.print("Ready 1");
    oled1.sendBuffer();

    tcaSelect(1);
    oled2.begin();
    oled2.enableUTF8Print();
    oled2.setFont(u8g2_font_unifont_t_korean2);
    oled2.setFontDirection(0);
    oled2.setFontMode(1);
    oled2.clearBuffer();
    oled2.setCursor(0, 16);
    oled2.print("Ready 2");
    oled2.sendBuffer();

    tcaSelect(2);
    oled3.begin();
    oled3.enableUTF8Print();
    oled3.setFont(u8g2_font_unifont_t_korean2);
    oled3.setFontDirection(0);
    oled3.setFontMode(1);
    oled3.clearBuffer();
    oled3.setCursor(0, 16);
    oled3.print("Ready 3");
    oled3.sendBuffer();

    delay(1000);

    displayTextAcrossOLEDs("블루투스 연결을 기다리고 있어요. 가사 연결을 위해 전용 앱과 연결해주세요.");
}

void loop() {
    if (SerialBT.available()) {
        char receivedChar = SerialBT.read();

        if (receivedChar == '\n' || receivedChar == '\r') {
            if (dataIndex > 0) {
                receivedData[dataIndex] = '\0';

                Serial.print("[DEBUG] received message: ");
                Serial.println(receivedData);
                displayTextAcrossOLEDs(receivedData);

                dataIndex = 0;
            }
        } else {
            if (dataIndex < 127) {
                receivedData[dataIndex] = receivedChar;
                dataIndex++;
            }
        }
    }
}

void tcaSelect(uint8_t channel) {
    if (channel > 7) return;

    Wire.beginTransmission(TCA9548A_ADDRESS);
    Wire.write(1 << channel);
    Wire.endTransmission();
}

void displayTextAcrossOLEDs(String text) {
    tcaSelect(0);

    struct CharInfo {
        char chars[5];
        int x;
        int y;
        int width;
    };

    CharInfo* charInfos = (CharInfo*)malloc(sizeof(CharInfo) * 150);
    if (charInfos == NULL) {
        Serial.println("[ERROR] 메모리 할당 실패");
        return;
    }

    int charCount = 0;
    int currentLine = 0;
    int currentX = 0;

    const int MARGIN = 2;

    for (int idx = 0; idx < text.length() && charCount < 150;) {
        int charLen = 1;
        unsigned char c = text[idx];
        if ((c & 0x80) == 0)
            charLen = 1;
        else if ((c & 0xE0) == 0xC0)
            charLen = 2;
        else if ((c & 0xF0) == 0xE0)
            charLen = 3;
        else if ((c & 0xF8) == 0xF0)
            charLen = 4;

        for (int i = 0; i < charLen && i < 4; i++) {
            charInfos[charCount].chars[i] = text[idx + i];
        }
        charInfos[charCount].chars[charLen] = '\0';

        int charWidth = oled1.getUTF8Width(charInfos[charCount].chars);

        bool crossesBoundary = false;

        if (currentX < OLED_WIDTH && currentX + charWidth > OLED_WIDTH - MARGIN) {
            crossesBoundary = true;
        } else if (currentX < OLED_WIDTH * 2 && currentX + charWidth > OLED_WIDTH * 2 - MARGIN) {
            crossesBoundary = true;
        }

        if (crossesBoundary) {
            if (currentX < OLED_WIDTH) {
                currentX = OLED_WIDTH;
            } else if (currentX < OLED_WIDTH * 2) {
                currentX = OLED_WIDTH * 2;
            }
        }

        if (currentX + charWidth > OLED_WIDTH * 3) {
            currentLine++;
            currentX = 0;
            if (currentLine >= 8) break;
        }

        charInfos[charCount].x = currentX;
        charInfos[charCount].y = 20 + (currentLine * 20);
        charInfos[charCount].width = charWidth;

        currentX += charWidth;
        charCount++;
        idx += charLen;
    }

    tcaSelect(0);
    oled1.clearBuffer();
    int oled1Count = 0;
    for (int i = 0; i < charCount; i++) {
        if (charInfos[i].x >= 0 && charInfos[i].x + charInfos[i].width <= OLED_WIDTH) {
            oled1.setCursor(charInfos[i].x, charInfos[i].y);
            oled1.print(charInfos[i].chars);
            oled1Count++;
        }
    }
    oled1.sendBuffer();

    tcaSelect(1);
    oled2.clearBuffer();
    int oled2Count = 0;
    for (int i = 0; i < charCount; i++) {
        if (charInfos[i].x >= OLED_WIDTH && charInfos[i].x + charInfos[i].width <= OLED_WIDTH * 2) {
            oled2.setCursor(charInfos[i].x - OLED_WIDTH, charInfos[i].y);
            oled2.print(charInfos[i].chars);
            oled2Count++;
        }
    }
    oled2.sendBuffer();

    tcaSelect(2);
    oled3.clearBuffer();
    int oled3Count = 0;
    for (int i = 0; i < charCount; i++) {
        if (charInfos[i].x >= OLED_WIDTH * 2 && charInfos[i].x + charInfos[i].width <= OLED_WIDTH * 3) {
            oled3.setCursor(charInfos[i].x - OLED_WIDTH * 2, charInfos[i].y);
            oled3.print(charInfos[i].chars);
            oled3Count++;
        }
    }
    oled3.sendBuffer();

    free(charInfos);
}