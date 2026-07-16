#include <SDL2/SDL.h>
#include <cstdio>

#include "Arduino.h"
#include "TFT_eSPI.h"
#include "app.h"

namespace {
constexpr int kScale = 2;
constexpr int kNativeW = TFT_WIDTH;
constexpr int kNativeH = TFT_HEIGHT;

uint32_t rgb565ToArgb(uint16_t c) {
  uint8_t r = (uint8_t)(((c >> 11) & 0x1F) * 255 / 31);
  uint8_t g = (uint8_t)(((c >> 5) & 0x3F) * 255 / 63);
  uint8_t b = (uint8_t)((c & 0x1F) * 255 / 31);
  return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
}  // namespace

int main(int, char**) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  TFT_eSPI tft;
  appSetup(tft);

  const int winW = tft.width() * kScale;
  const int winH = tft.height() * kScale;

  SDL_Window* window = SDL_CreateWindow(
      "BTC-Mine Simulator (ESP32-2432S028)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winW,
      winH, SDL_WINDOW_SHOWN);
  if (!window) {
    std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Renderer* renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    renderer = SDL_CreateRenderer(window, -1, 0);
  }
  SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING, tft.width(), tft.height());

  std::printf("BTC-Mine simulator gestart — %dx%d (scale %dx)\n", tft.width(), tft.height(),
              kScale);
  std::printf("Muis = touch. Venster sluiten of ESC = stoppen.\n");

  bool running = true;
  uint32_t lastLoop = SDL_GetTicks();
  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) running = false;
      if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = false;

      if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION) {
        int mx = 0, my = 0;
        uint32_t buttons = SDL_GetMouseState(&mx, &my);
        bool pressed = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        tft.simSetTouch(pressed, (int16_t)(mx / kScale), (int16_t)(my / kScale));
      }
    }

    uint32_t now = SDL_GetTicks();
    // Roughly match Arduino loop cadence without blocking the UI thread too hard.
    if (now - lastLoop >= 16) {
      appLoop(tft);
      lastLoop = now;
    }

    // Present logical framebuffer in current rotation
    void* pixels = nullptr;
    int pitch = 0;
    if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) == 0) {
      auto* out = static_cast<uint32_t*>(pixels);
      const int lw = tft.width();
      const int lh = tft.height();
      for (int y = 0; y < lh; ++y) {
        for (int x = 0; x < lw; ++x) {
          // Map logical (x,y) back to native framebuffer via rotation inverse used by setPixelRaw
          int nx = x, ny = y;
          switch (tft.getRotation()) {
            case 1:
              nx = y;
              ny = kNativeH - 1 - x;
              break;
            case 2:
              nx = kNativeW - 1 - x;
              ny = kNativeH - 1 - y;
              break;
            case 3:
              nx = kNativeW - 1 - y;
              ny = x;
              break;
            default:
              break;
          }
          uint16_t c = tft.framebuffer()[ny * kNativeW + nx];
          out[y * (pitch / 4) + x] = rgb565ToArgb(c);
        }
      }
      SDL_UnlockTexture(texture);
    }

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
