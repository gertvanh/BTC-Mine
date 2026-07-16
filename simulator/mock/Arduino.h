#pragma once

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>

#ifdef _WIN32
#include <windows.h>
static inline unsigned long millis() { return (unsigned long)GetTickCount(); }
static inline void delay(unsigned long ms) { Sleep(ms); }
#else
#include <time.h>
static inline unsigned long millis() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (unsigned long)(ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL);
}
static inline void delay(unsigned long ms) {
  struct timespec ts = {(time_t)(ms / 1000), (long)((ms % 1000) * 1000000L)};
  nanosleep(&ts, nullptr);
}
#endif

#define F(x) (x)
#define HIGH 0x1
#define LOW 0x0
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

#ifndef TFT_BL
#define TFT_BL 21
#endif
#ifndef TFT_BACKLIGHT_ON
#define TFT_BACKLIGHT_ON HIGH
#endif

typedef std::string String;

struct _SimSerial {
  void begin(int) {}
  void flush() { fflush(stdout); }
  void print(const char* s) { fputs(s, stdout); }
  void print(int v) { printf("%d", v); }
  void print(unsigned v) { printf("%u", v); }
  void print(long v) { printf("%ld", v); }
  void print(float v, int d = 2) { printf("%.*f", d, (double)v); }
  void print(const String& s) { fputs(s.c_str(), stdout); }
  void println(const char* s = "") { printf("%s\n", s); }
  void println(int v) { printf("%d\n", v); }
  void println(unsigned v) { printf("%u\n", v); }
  void println(long v) { printf("%ld\n", v); }
  void println(float v, int d = 2) { printf("%.*f\n", d, (double)v); }
  void println(const String& s) { printf("%s\n", s.c_str()); }
  void printf(const char* fmt, ...) {
    va_list a;
    va_start(a, fmt);
    vprintf(fmt, a);
    va_end(a);
  }
};

extern _SimSerial SimSerial;
#define Serial SimSerial

static inline void pinMode(int, int) {}
static inline void digitalWrite(int, int) {}
static inline int digitalRead(int) { return LOW; }

template <typename T>
static inline T max(T a, T b) {
  return a > b ? a : b;
}
template <typename T>
static inline T min(T a, T b) {
  return a < b ? a : b;
}
template <typename T>
static inline T abs(T v) {
  return v < 0 ? -v : v;
}
