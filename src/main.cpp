#include <Arduino.h>
#include <TFT_eSPI.h>

#include "app.h"
#include "version.h"
#include "wifi_ota.h"

TFT_eSPI tft;

namespace {
bool uiConnected = false;
unsigned long lastUiMs = 0;
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

  appSetup(tft);
  wifiOtaBegin();
}

void loop() {
  wifiOtaLoop();
  appLoop(tft);

  const unsigned long now = millis();
  if (now - lastUiMs >= 1000) {
    lastUiMs = now;
    const bool connected = wifiOtaConnected();
    if (connected != uiConnected) {
      uiConnected = connected;
      if (connected) {
        appShowStatus(tft, "BTC-Mine OTA", wifiOtaIpString().c_str(), FW_VERSION);
      } else {
        appShowStatus(tft, "BTC-Mine", "WiFi...", FW_VERSION);
      }
    }
  }

  delay(16);
}
