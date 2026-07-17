#include "app.h"

#include "version.h"

void appSetup(TFT_eSPI& tft) {
  tft.init();
  tft.setRotation(1);  // landscape 320x240 on ESP32-2432S028
  appShowStatus(tft, "BTC-Mine", "WiFi...", FW_VERSION);
  Serial.println("ESP32-2432S028 WiFiManager + DHCP");
}

void appLoop(TFT_eSPI& tft) { (void)tft; }

void appShowStatus(TFT_eSPI& tft, const char* line1, const char* line2, const char* line3) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  const int16_t cx = tft.width() / 2;
  const int16_t cy = tft.height() / 2;
  if (line1 && line1[0]) tft.drawString(line1, cx, cy - 28);
  if (line2 && line2[0]) tft.drawString(line2, cx, cy);
  if (line3 && line3[0]) tft.drawString(line3, cx, cy + 28);
}
