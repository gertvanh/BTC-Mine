#pragma once

#include "Arduino.h"
#include <cstdint>

// RGB565 palette (TFT_eSPI compatible)
#define TFT_BLACK       0x0000
#define TFT_NAVY        0x000F
#define TFT_DARKGREEN   0x03E0
#define TFT_DARKCYAN    0x03EF
#define TFT_MAROON      0x7800
#define TFT_PURPLE      0x780F
#define TFT_OLIVE       0x7BE0
#define TFT_LIGHTGREY   0xD69A
#define TFT_DARKGREY    0x7BEF
#define TFT_BLUE        0x001F
#define TFT_GREEN       0x07E0
#define TFT_CYAN        0x07FF
#define TFT_RED         0xF800
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_WHITE       0xFFFF
#define TFT_ORANGE      0xFDA0
#define TFT_GREENYELLOW 0xB7E0
#define TFT_PINK        0xFE19
#define TFT_BROWN       0x9A60
#define TFT_GOLD        0xFEA0
#define TFT_SILVER      0xC618
#define TFT_SKYBLUE     0x867D
#define TFT_VIOLET      0x915C

// Text datums
#define TL_DATUM 0
#define TC_DATUM 1
#define TR_DATUM 2
#define ML_DATUM 3
#define MC_DATUM 4
#define MR_DATUM 5
#define BL_DATUM 6
#define BC_DATUM 7
#define BR_DATUM 8
#define L_BASELINE 9
#define C_BASELINE 10
#define R_BASELINE 11

#ifndef TFT_WIDTH
#define TFT_WIDTH 240
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT 320
#endif

class TFT_eSPI {
 public:
  TFT_eSPI(int16_t = TFT_WIDTH, int16_t = TFT_HEIGHT);

  void init();
  void begin() { init(); }
  void setRotation(uint8_t r);
  uint8_t getRotation() const { return _rotation; }

  int16_t width() const { return _width; }
  int16_t height() const { return _height; }

  void fillScreen(uint16_t color);
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
  void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
  void drawPixel(int32_t x, int32_t y, uint16_t color);
  void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color);
  void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color);
  void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color);
  void drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color);
  void fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color);
  void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color);
  void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color);

  void setTextColor(uint16_t fg, uint16_t bg = TFT_BLACK);
  void setTextDatum(uint8_t datum);
  void setTextSize(uint8_t size);
  void setCursor(int16_t x, int16_t y);
  int16_t drawString(const char* str, int32_t x, int32_t y, uint8_t font = 2);
  int16_t drawString(const String& str, int32_t x, int32_t y, uint8_t font = 2);
  int16_t drawCentreString(const char* str, int32_t x, int32_t y, uint8_t font = 2);
  int16_t drawNumber(long n, int32_t x, int32_t y, uint8_t font = 2);
  int16_t drawFloat(float f, uint8_t dp, int32_t x, int32_t y, uint8_t font = 2);
  void print(const char* str);
  void println(const char* str = "");

  // Touch stub: filled by simulator host each frame
  bool getTouch(uint16_t* x, uint16_t* y, uint16_t threshold = 600);

  // Host helpers
  const uint16_t* framebuffer() const { return _fb; }
  void simSetTouch(bool pressed, int16_t x, int16_t y);

 private:
  void setPixelRaw(int32_t x, int32_t y, uint16_t color);
  int16_t textWidth(const char* str, uint8_t size) const;
  int16_t fontHeight(uint8_t size) const;
  void drawChar(int32_t x, int32_t y, char c, uint16_t fg, uint16_t bg, uint8_t size);

  uint16_t _fb[TFT_WIDTH * TFT_HEIGHT];
  int16_t _nativeW;
  int16_t _nativeH;
  int16_t _width;
  int16_t _height;
  uint8_t _rotation;
  uint16_t _textfg;
  uint16_t _textbg;
  uint8_t _datum;
  uint8_t _textsize;
  int16_t _cursorX;
  int16_t _cursorY;
  bool _touchPressed;
  int16_t _touchX;
  int16_t _touchY;
};
