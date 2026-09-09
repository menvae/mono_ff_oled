#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define ROW_BYTES     ((SCREEN_WIDTH + 7) / 8)      // 16
#define FRAME_SIZE    (ROW_BYTES * SCREEN_HEIGHT)    // 1024
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint8_t frameBuffer[FRAME_SIZE];

void setup() {
    Serial.setRxBufferSize(2048);

    Serial.begin(921600);

    Wire.begin(21, 22);

    Wire.setClock(800000);

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        for (;;);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Awaiting...");
    display.display();
}

void loop() {
    if (Serial.available() >= FRAME_SIZE) {
        Serial.readBytes(display.getBuffer(), FRAME_SIZE);

        display.display();

        while (Serial.available()) {
            Serial.read();
        }

        Serial.println("OK");
    }
}
