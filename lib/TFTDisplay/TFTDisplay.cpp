// lib/TFTDisplay/TFTDisplay.cpp
//
// Hardware-Layer fuer das ST7735 TFT (z.B. 128x160).
// Dieses Modul kapselt die konkrete Display-Library (Adafruit_ST7735 o.a.)
// und stellt einfache Zeichenprimitive fuer die GUI bereit.
//
// Wichtig:
// - KEINE Aenderung der oeffentlichen API-Signaturen (initDisplay bleibt bool).
// - GUI nutzt fillRectRGB() fuer teilweises Loeschen (weniger Flackern).

#include "TFTDisplay.h"
#include <Arduino.h>

// Falls Sie Adafruit nutzen (wie bisher in Ihrem Projekt):
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// Pins kommen aus Ihrer globalen config.h (wie bisher bei Ihnen)
#include <config.h>

// -----------------------------------------------------------------------------
// Internes Display-Objekt
// -----------------------------------------------------------------------------
static Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

// -----------------------------------------------------------------------------
// Hilfsfunktion: RGB888 -> RGB565 (16-bit)
// -----------------------------------------------------------------------------
static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) |
                    ((g & 0xFC) << 3) |
                    ((b & 0xF8) >> 3));
}

/**
 * @brief Initialisiert das Display.
 *
 * @return true wenn Initialisierung plausibel durchlief
 *
 * Hinweise:
 * - Je nach ST7735-Board kann INITR_BLACKTAB/GREENTAB/REDTAB notwendig sein.
 * - Rotation (0..3) bestimmt Ausrichtung. Wenn die Anzeige "komisch" ist,
 *   ist Rotation + initR(Tab) das erste, was man prueft.
 */
bool initDisplay() {
  // Backlight einschalten nur bei der esp32 var notwendig da bei eth01 hardgecoded ist
  //pinMode(TFT_BL, OUTPUT);
  //digitalWrite(TFT_BL, HIGH);

  // SPI starten (kein MISO => -1)
  SPI.begin(TFT_SCK, -1, TFT_MOSI);

  // Bei vielen ST7735 Modulen ist BLACKTAB oder GREENTAB korrekt.
  // Wenn Farben vertauscht/Offset falsch: hier wechseln.
  tft.initR(INITR_BLACKTAB);

  // Rotation an Ihr Layout anpassen (0..3)
  // Ihre vorherige Einstellung beibehalten (typisch 1 fuer Landscape).
  tft.setRotation(1);

  // Mach nen BIT Test
  //runBit();

  return true;
}

/**
 * @brief Optionaler Built-In-Test (IBIT).
 *
 * Zweck:
 * - Sichtbarer Funktionstest direkt nach Boot: Farben + Text.
 * - Danach wird wieder schwarz gemacht, damit die GUI sauber starten kann.
 */
void runBit() {
  // Basis: schwarz
  tft.fillScreen(rgb565(0, 0, 0));

  // Einfache Testflächen
  tft.fillRect(0, 0, tft.width(), tft.height() / 3, rgb565(255, 0, 0));
  tft.fillRect(0, tft.height() / 3, tft.width(), tft.height() / 3, rgb565(0, 255, 0));
  tft.fillRect(0, 2 * (tft.height() / 3), tft.width(), tft.height() / 3, rgb565(0, 0, 255));

  // Testtext
  tft.setCursor(4, 4);
  tft.setTextSize(1);
  tft.setTextColor(rgb565(255, 255, 255));
  tft.print("IBIT");

  delay(250);

  // Zurueck auf schwarz (GUI zeichnet danach)
  tft.fillScreen(rgb565(0, 0, 0));
}

/**
 * @brief Loescht den kompletten Screen (schwarz).
 *
 * Hinweis:
 * - Fuer flackerarmes UI bevorzugt die GUI fillRectRGB() auf Teilbereichen.
 */
void clearDisplay() {
  tft.fillScreen(rgb565(0, 0, 0));
}

/**
 * @brief Zeichnet Text (transparent, ohne Hintergrundfarbe).
 *
 * Hintergrund:
 * - Wir loeschen Bereiche vor dem Neuzeichnen per fillRectRGB().
 * - Daher ist kein Text-Hintergrund noetig.
 */
void drawText(const char* text, int16_t x, int16_t y, uint8_t size,
              uint8_t r, uint8_t g, uint8_t b) {
  tft.setCursor(x, y);
  tft.setTextSize(size);
  tft.setTextColor(rgb565(r, g, b)); // transparent (kein bg)
  tft.print(text);
}

/**
 * @brief Liefert die aktuelle Displaygroesse (abhaengig von Rotation).
 */
void getDisplaySize(int16_t &w, int16_t &h) {
  w = tft.width();
  h = tft.height();
}

/**
 * @brief Zeichnet eine Linie in RGB-Farbe.
 */
void drawLineRGB(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                 uint8_t r, uint8_t g, uint8_t b) {
  tft.drawLine(x0, y0, x1, y1, rgb565(r, g, b));
}

/**
 * @brief Fuellt ein Rechteck in RGB-Farbe.
 *
 * Wichtig fuer Anti-Flicker:
 * - GUI loescht damit gezielt Header/Value/Footer statt Full-Screen-Clear.
 */
void fillRectRGB(int16_t x, int16_t y, int16_t w, int16_t h,
                 uint8_t r, uint8_t g, uint8_t b) {
  tft.fillRect(x, y, w, h, rgb565(r, g, b));
}
