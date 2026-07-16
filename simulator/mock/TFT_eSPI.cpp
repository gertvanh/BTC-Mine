#include "TFT_eSPI.h"
#include <algorithm>
#include <cmath>

// Classic 5x7 font (ASCII 32..126), column-major bits (LSB = top)
static const uint8_t FONT5X7[][5] = {
  {0x00,0x00,0x00,0x00,0x00}, // space
  {0x00,0x00,0x5F,0x00,0x00}, // !
  {0x00,0x07,0x00,0x07,0x00}, // "
  {0x14,0x7F,0x14,0x7F,0x14}, // #
  {0x24,0x2A,0x7F,0x2A,0x12}, // $
  {0x23,0x13,0x08,0x64,0x62}, // %
  {0x36,0x49,0x55,0x22,0x50}, // &
  {0x00,0x05,0x03,0x00,0x00}, // '
  {0x00,0x1C,0x22,0x41,0x00}, // (
  {0x00,0x41,0x22,0x1C,0x00}, // )
  {0x08,0x2A,0x1C,0x2A,0x08}, // *
  {0x08,0x08,0x3E,0x08,0x08}, // +
  {0x00,0x50,0x30,0x00,0x00}, // ,
  {0x08,0x08,0x08,0x08,0x08}, // -
  {0x00,0x60,0x60,0x00,0x00}, // .
  {0x20,0x10,0x08,0x04,0x02}, // /
  {0x3E,0x51,0x49,0x45,0x3E}, // 0
  {0x00,0x42,0x7F,0x40,0x00}, // 1
  {0x42,0x61,0x51,0x49,0x46}, // 2
  {0x21,0x41,0x45,0x4B,0x31}, // 3
  {0x18,0x14,0x12,0x7F,0x10}, // 4
  {0x27,0x45,0x45,0x45,0x39}, // 5
  {0x3C,0x4A,0x49,0x49,0x30}, // 6
  {0x01,0x71,0x09,0x05,0x03}, // 7
  {0x36,0x49,0x49,0x49,0x36}, // 8
  {0x06,0x49,0x49,0x29,0x1E}, // 9
  {0x00,0x36,0x36,0x00,0x00}, // :
  {0x00,0x56,0x36,0x00,0x00}, // ;
  {0x00,0x08,0x14,0x22,0x41}, // <
  {0x14,0x14,0x14,0x14,0x14}, // =
  {0x41,0x22,0x14,0x08,0x00}, // >
  {0x02,0x01,0x51,0x09,0x06}, // ?
  {0x32,0x49,0x79,0x41,0x3E}, // @
  {0x7E,0x11,0x11,0x11,0x7E}, // A
  {0x7F,0x49,0x49,0x49,0x36}, // B
  {0x3E,0x41,0x41,0x41,0x22}, // C
  {0x7F,0x41,0x41,0x22,0x1C}, // D
  {0x7F,0x49,0x49,0x49,0x41}, // E
  {0x7F,0x09,0x09,0x01,0x01}, // F
  {0x3E,0x41,0x41,0x51,0x32}, // G
  {0x7F,0x08,0x08,0x08,0x7F}, // H
  {0x00,0x41,0x7F,0x41,0x00}, // I
  {0x20,0x40,0x41,0x3F,0x01}, // J
  {0x7F,0x08,0x14,0x22,0x41}, // K
  {0x7F,0x40,0x40,0x40,0x40}, // L
  {0x7F,0x02,0x04,0x02,0x7F}, // M
  {0x7F,0x04,0x08,0x10,0x7F}, // N
  {0x3E,0x41,0x41,0x41,0x3E}, // O
  {0x7F,0x09,0x09,0x09,0x06}, // P
  {0x3E,0x41,0x51,0x21,0x5E}, // Q
  {0x7F,0x09,0x19,0x29,0x46}, // R
  {0x46,0x49,0x49,0x49,0x31}, // S
  {0x01,0x01,0x7F,0x01,0x01}, // T
  {0x3F,0x40,0x40,0x40,0x3F}, // U
  {0x1F,0x20,0x40,0x20,0x1F}, // V
  {0x7F,0x20,0x18,0x20,0x7F}, // W
  {0x63,0x14,0x08,0x14,0x63}, // X
  {0x03,0x04,0x78,0x04,0x03}, // Y
  {0x61,0x51,0x49,0x45,0x43}, // Z
  {0x00,0x00,0x7F,0x41,0x41}, // [
  {0x02,0x04,0x08,0x10,0x20}, // backslash
  {0x41,0x41,0x7F,0x00,0x00}, // ]
  {0x04,0x02,0x01,0x02,0x04}, // ^
  {0x40,0x40,0x40,0x40,0x40}, // _
  {0x00,0x01,0x02,0x04,0x00}, // `
  {0x20,0x54,0x54,0x54,0x78}, // a
  {0x7F,0x48,0x44,0x44,0x38}, // b
  {0x38,0x44,0x44,0x44,0x20}, // c
  {0x38,0x44,0x44,0x48,0x7F}, // d
  {0x38,0x54,0x54,0x54,0x18}, // e
  {0x08,0x7E,0x09,0x01,0x02}, // f
  {0x08,0x14,0x54,0x54,0x3C}, // g
  {0x7F,0x08,0x04,0x04,0x78}, // h
  {0x00,0x44,0x7D,0x40,0x00}, // i
  {0x20,0x40,0x44,0x3D,0x00}, // j
  {0x7F,0x10,0x28,0x44,0x00}, // k
  {0x00,0x41,0x7F,0x40,0x00}, // l
  {0x7C,0x04,0x18,0x04,0x78}, // m
  {0x7C,0x08,0x04,0x04,0x78}, // n
  {0x38,0x44,0x44,0x44,0x38}, // o
  {0x7C,0x14,0x14,0x14,0x08}, // p
  {0x08,0x14,0x14,0x18,0x7C}, // q
  {0x7C,0x08,0x04,0x04,0x08}, // r
  {0x48,0x54,0x54,0x54,0x24}, // s
  {0x04,0x3F,0x44,0x40,0x20}, // t
  {0x3C,0x40,0x40,0x20,0x7C}, // u
  {0x1C,0x20,0x40,0x20,0x1C}, // v
  {0x3C,0x40,0x30,0x40,0x3C}, // w
  {0x44,0x28,0x10,0x28,0x44}, // x
  {0x0C,0x50,0x50,0x50,0x3C}, // y
  {0x44,0x64,0x54,0x4C,0x44}, // z
  {0x00,0x08,0x36,0x41,0x00}, // {
  {0x00,0x00,0x7F,0x00,0x00}, // |
  {0x00,0x41,0x36,0x08,0x00}, // }
  {0x08,0x04,0x08,0x10,0x08}, // ~
};

TFT_eSPI::TFT_eSPI(int16_t w, int16_t h)
    : _nativeW(w),
      _nativeH(h),
      _width(w),
      _height(h),
      _rotation(0),
      _textfg(TFT_WHITE),
      _textbg(TFT_BLACK),
      _datum(TL_DATUM),
      _textsize(2),
      _cursorX(0),
      _cursorY(0),
      _touchPressed(false),
      _touchX(0),
      _touchY(0) {
  std::fill(_fb, _fb + (TFT_WIDTH * TFT_HEIGHT), (uint16_t)TFT_BLACK);
}

void TFT_eSPI::init() {
  setRotation(0);
  fillScreen(TFT_BLACK);
}

void TFT_eSPI::setRotation(uint8_t r) {
  _rotation = r & 3;
  if (_rotation & 1) {
    _width = _nativeH;
    _height = _nativeW;
  } else {
    _width = _nativeW;
    _height = _nativeH;
  }
}

void TFT_eSPI::setPixelRaw(int32_t x, int32_t y, uint16_t color) {
  if (x < 0 || y < 0 || x >= _width || y >= _height) return;

  int32_t nx = x;
  int32_t ny = y;
  switch (_rotation) {
    case 1:
      nx = y;
      ny = _nativeH - 1 - x;
      break;
    case 2:
      nx = _nativeW - 1 - x;
      ny = _nativeH - 1 - y;
      break;
    case 3:
      nx = _nativeW - 1 - y;
      ny = x;
      break;
    default:
      break;
  }
  if (nx < 0 || ny < 0 || nx >= _nativeW || ny >= _nativeH) return;
  _fb[ny * _nativeW + nx] = color;
}

void TFT_eSPI::drawPixel(int32_t x, int32_t y, uint16_t color) { setPixelRaw(x, y, color); }

void TFT_eSPI::fillScreen(uint16_t color) { fillRect(0, 0, _width, _height, color); }

void TFT_eSPI::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  for (int32_t yy = y; yy < y + h; ++yy)
    for (int32_t xx = x; xx < x + w; ++xx) setPixelRaw(xx, yy, color);
}

void TFT_eSPI::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x + w - 1, y, h, color);
}

void TFT_eSPI::drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) {
  for (int32_t i = 0; i < w; ++i) setPixelRaw(x + i, y, color);
}

void TFT_eSPI::drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) {
  for (int32_t i = 0; i < h; ++i) setPixelRaw(x, y + i, color);
}

void TFT_eSPI::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) {
  int32_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int32_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int32_t err = dx + dy;
  for (;;) {
    setPixelRaw(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    int32_t e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void TFT_eSPI::drawCircle(int32_t x0, int32_t y0, int32_t r, uint16_t color) {
  int32_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
  setPixelRaw(x0, y0 + r, color);
  setPixelRaw(x0, y0 - r, color);
  setPixelRaw(x0 + r, y0, color);
  setPixelRaw(x0 - r, y0, color);
  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    setPixelRaw(x0 + x, y0 + y, color);
    setPixelRaw(x0 - x, y0 + y, color);
    setPixelRaw(x0 + x, y0 - y, color);
    setPixelRaw(x0 - x, y0 - y, color);
    setPixelRaw(x0 + y, y0 + x, color);
    setPixelRaw(x0 - y, y0 + x, color);
    setPixelRaw(x0 + y, y0 - x, color);
    setPixelRaw(x0 - y, y0 - x, color);
  }
}

void TFT_eSPI::fillCircle(int32_t x0, int32_t y0, int32_t r, uint16_t color) {
  drawFastVLine(x0, y0 - r, 2 * r + 1, color);
  int32_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    drawFastVLine(x0 + x, y0 - y, 2 * y + 1, color);
    drawFastVLine(x0 - x, y0 - y, 2 * y + 1, color);
    drawFastVLine(x0 + y, y0 - x, 2 * x + 1, color);
    drawFastVLine(x0 - y, y0 - x, 2 * x + 1, color);
  }
}

void TFT_eSPI::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) {
  drawFastHLine(x + r, y, w - 2 * r, color);
  drawFastHLine(x + r, y + h - 1, w - 2 * r, color);
  drawFastVLine(x, y + r, h - 2 * r, color);
  drawFastVLine(x + w - 1, y + r, h - 2 * r, color);
  // corner arcs (approx via circle segments)
  for (int32_t i = 0; i <= r; ++i) {
    for (int32_t j = 0; j <= r; ++j) {
      if (i * i + j * j <= r * r && i * i + j * j > (r - 1) * (r - 1)) {
        setPixelRaw(x + r - i, y + r - j, color);
        setPixelRaw(x + w - 1 - r + i, y + r - j, color);
        setPixelRaw(x + r - i, y + h - 1 - r + j, color);
        setPixelRaw(x + w - 1 - r + i, y + h - 1 - r + j, color);
      }
    }
  }
}

void TFT_eSPI::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) {
  fillRect(x + r, y, w - 2 * r, h, color);
  fillRect(x, y + r, r, h - 2 * r, color);
  fillRect(x + w - r, y + r, r, h - 2 * r, color);
  fillCircle(x + r, y + r, r, color);
  fillCircle(x + w - 1 - r, y + r, r, color);
  fillCircle(x + r, y + h - 1 - r, r, color);
  fillCircle(x + w - 1 - r, y + h - 1 - r, r, color);
}

void TFT_eSPI::setTextColor(uint16_t fg, uint16_t bg) {
  _textfg = fg;
  _textbg = bg;
}

void TFT_eSPI::setTextDatum(uint8_t datum) { _datum = datum; }

void TFT_eSPI::setTextSize(uint8_t size) { _textsize = size ? size : 1; }

void TFT_eSPI::setCursor(int16_t x, int16_t y) {
  _cursorX = x;
  _cursorY = y;
}

int16_t TFT_eSPI::fontHeight(uint8_t size) const { return (int16_t)(8 * size); }

int16_t TFT_eSPI::textWidth(const char* str, uint8_t size) const {
  return (int16_t)(strlen(str) * 6 * size);
}

void TFT_eSPI::drawChar(int32_t x, int32_t y, char c, uint16_t fg, uint16_t bg, uint8_t size) {
  if (c < 32 || c > 126) c = '?';
  const uint8_t* glyph = FONT5X7[c - 32];
  for (int32_t i = 0; i < 5; ++i) {
    uint8_t line = glyph[i];
    for (int32_t j = 0; j < 7; ++j) {
      uint16_t color = (line & (1 << j)) ? fg : bg;
      if (color == bg && bg == fg) continue;
      if (size == 1) {
        if (line & (1 << j)) setPixelRaw(x + i, y + j, fg);
      } else {
        if (line & (1 << j)) fillRect(x + i * size, y + j * size, size, size, fg);
      }
    }
  }
}

int16_t TFT_eSPI::drawString(const char* str, int32_t x, int32_t y, uint8_t font) {
  (void)font;
  uint8_t size = _textsize;
  int16_t tw = textWidth(str, size);
  int16_t th = fontHeight(size);
  int32_t ox = x;
  int32_t oy = y;
  switch (_datum) {
    case TC_DATUM:
    case MC_DATUM:
    case BC_DATUM:
    case C_BASELINE:
      ox -= tw / 2;
      break;
    case TR_DATUM:
    case MR_DATUM:
    case BR_DATUM:
    case R_BASELINE:
      ox -= tw;
      break;
    default:
      break;
  }
  switch (_datum) {
    case ML_DATUM:
    case MC_DATUM:
    case MR_DATUM:
      oy -= th / 2;
      break;
    case BL_DATUM:
    case BC_DATUM:
    case BR_DATUM:
      oy -= th;
      break;
    default:
      break;
  }

  for (const char* p = str; *p; ++p) {
    if (_textbg != _textfg) fillRect(ox, oy, 6 * size, th, _textbg);
    drawChar(ox, oy, *p, _textfg, _textbg, size);
    ox += 6 * size;
  }
  return tw;
}

int16_t TFT_eSPI::drawString(const String& str, int32_t x, int32_t y, uint8_t font) {
  return drawString(str.c_str(), x, y, font);
}

int16_t TFT_eSPI::drawCentreString(const char* str, int32_t x, int32_t y, uint8_t font) {
  uint8_t old = _datum;
  _datum = TC_DATUM;
  int16_t w = drawString(str, x, y, font);
  _datum = old;
  return w;
}

int16_t TFT_eSPI::drawNumber(long n, int32_t x, int32_t y, uint8_t font) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%ld", n);
  return drawString(buf, x, y, font);
}

int16_t TFT_eSPI::drawFloat(float f, uint8_t dp, int32_t x, int32_t y, uint8_t font) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%.*f", (int)dp, (double)f);
  return drawString(buf, x, y, font);
}

void TFT_eSPI::print(const char* str) {
  drawString(str, _cursorX, _cursorY, 2);
  _cursorX += textWidth(str, _textsize);
}

void TFT_eSPI::println(const char* str) {
  print(str);
  _cursorX = 0;
  _cursorY += fontHeight(_textsize);
}

bool TFT_eSPI::getTouch(uint16_t* x, uint16_t* y, uint16_t) {
  if (!_touchPressed) return false;
  if (x) *x = (uint16_t)_touchX;
  if (y) *y = (uint16_t)_touchY;
  return true;
}

void TFT_eSPI::simSetTouch(bool pressed, int16_t x, int16_t y) {
  _touchPressed = pressed;
  _touchX = x;
  _touchY = y;
}
