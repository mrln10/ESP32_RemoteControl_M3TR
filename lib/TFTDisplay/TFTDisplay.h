#pragma once
#include <Arduino.h>
#include <stdint.h>

bool initDisplay();
void clearDisplay();
void runBit();

/**
 * Zeichnet Text mit vollständiger Kontrolle
 *
 * @param text      Text (C-String)
 * @param x         X-Position (Pixel)
 * @param y         Y-Position (Pixel)
 * @param size      Textgröße (1 = klein)
 * @param r,g,b     Textfarbe (RGB 0..255)
 */
void drawText(
  const char* text,
  int16_t x,
  int16_t y,
  uint8_t size,
  uint8_t r,
  uint8_t g,
  uint8_t b
);

void getDisplaySize(int16_t &w, int16_t &h);

void drawLineRGB(
  int16_t x0, int16_t y0,
  int16_t x1, int16_t y1,
  uint8_t r, uint8_t g, uint8_t b
);

void fillRectRGB(int16_t x, int16_t y, int16_t w, int16_t h,
                 uint8_t r, uint8_t g, uint8_t b);

