#include <Arduino.h>
#include <TFT_eSPI.h>

#include "app.h"
#include "version.h"
#include "wifi_ota.h"

TFT_eSPI tft;

namespace {
String lastStatus;
unsigned long lastUiMs = 0;
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

  appSetup(tft);
  appShowStatus(tft, "BTC-Mine", "WiFi setup...", FW_VERSION);
  wifiOtaBegin();
}

void loop() {
  wifiOtaLoop();
  appLoop(tft);

  const unsigned long now = millis();
  if (now - lastUiMs >= 500) {
    lastUiMs = now;

    String line1 = "BTC-Mine";
    String line2 = wifiOtaStatusLine();
    String line3 = FW_VERSION;

    if (wifiOtaConnected()) {
      line1 = "BTC-Mine";
      line2 = wifiOtaIpString();
      if (wifiOtaSsid().length() > 0) {
        line3 = wifiOtaSsid() + " / " + String(FW_VERSION);
      }
    } else if (wifiOtaPortalActive()) {
      line1 = "WiFi setup";
      line2 = "BTC-Mine-setup";
      line3 = "open 192.168.4.1";
    }

    const String key = line1 + "|" + line2 + "|" + line3;
    if (key != lastStatus) {
      lastStatus = key;
      appShowStatus(tft, line1.c_str(), line2.c_str(), line3.c_str());
    }
  }

  delay(16);
}
