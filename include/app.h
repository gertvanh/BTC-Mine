#pragma once

#include <TFT_eSPI.h>

// Shared UI entry points for ESP32 firmware and desktop simulator.
void appSetup(TFT_eSPI& tft);
void appLoop(TFT_eSPI& tft);
void appShowStatus(TFT_eSPI& tft, const char* line1, const char* line2 = nullptr,
                   const char* line3 = nullptr);
