// lib/NavButtons/NavButtons.cpp
//
// Zwei Navigationstaster (LEFT/RIGHT) mit:
// - INPUT_PULLUP (Taster gegen GND)
// - Entprellung (Debounce)
// - Short-Press Event (bei Loslassen)
// - Long-Press Event (einmalig nach Haltezeit)
// - Garantie: Long-Press loest KEIN Short-Press aus
//
// Designziel:
// - GUI und main.cpp sollen nur Events abfragen ("getLeftPressed()"),
//   nicht selbst entprellen oder Timings verwalten.

#include "NavButtons.h"
#include <Arduino.h>
#include <config.h>

// --------------------
// Tuning-Parameter
// --------------------
// Debounce: stabiler Zustand muss mindestens so lange unveraendert sein
static const uint32_t BTN_DEBOUNCE_MS  = 30;

// Long-Press: wenn Taste so lange gehalten wird, gibt es genau EIN Long-Event
static const uint32_t BTN_LONGPRESS_MS = 700;

// --------------------
// Interner Button-State
// --------------------
struct ButtonState {
  int lastStable;         // letzter stabiler Zustand (HIGH/LOW)
  int lastReading;        // letzter Rohwert von digitalRead()
  uint32_t lastChangeMs;  // Zeitpunkt der letzten Rohwert-Aenderung

  bool shortEvent;        // wird gesetzt, wenn Short-Press erkannt
  bool longEvent;         // wird gesetzt, wenn Long-Press erkannt

  uint32_t downMs;        // Zeitpunkt, wann stabil "pressed" erkannt wurde
  bool longFired;         // damit Long-Press nur einmal feuert
};

static ButtonState leftBtn;
static ButtonState rightBtn;

/**
 * @brief Initialisiert einen ButtonState und den zugehoerigen GPIO.
 */
static void initOne(ButtonState &b, int pin) {
  pinMode(pin, INPUT_PULLUP);

  int r = digitalRead(pin);
  b.lastReading = r;
  b.lastStable = r;
  b.lastChangeMs = millis();

  b.shortEvent = false;
  b.longEvent = false;

  b.downMs = 0;
  b.longFired = false;
}

/**
 * @brief Aktualisiert einen Button (Debounce + Event-Generierung).
 *
 * Logik (wichtig):
 * - Short-Press wird beim LOSLASSEN erzeugt, aber nur wenn kein Long-Press war.
 * - Long-Press wird erzeugt, sobald die Taste lange genug gehalten wird (einmalig).
 */
static void updateOne(ButtonState &b, int pin) {
  int r = digitalRead(pin);

  // Rohwert-Aenderung merken
  if (r != b.lastReading) {
    b.lastReading = r;
    b.lastChangeMs = millis();
  }

  // Nach Debounce-Zeit: stabilen Zustand uebernehmen
  if ((millis() - b.lastChangeMs) > BTN_DEBOUNCE_MS) {
    if (b.lastStable != b.lastReading) {
      b.lastStable = b.lastReading;

      if (b.lastStable == LOW) {
        // Taste wurde stabil gedrueckt
        b.downMs = millis();
        b.longFired = false;
      } else {
        // Taste wurde stabil losgelassen
        // -> Short nur, wenn kein Long-Press passiert ist
        if (b.downMs != 0 && !b.longFired) {
          b.shortEvent = true;
        }
        b.downMs = 0;
        b.longFired = false;
      }
    }
  }

  // Long-Press pruefen, solange gedrueckt
  if (b.lastStable == LOW && b.downMs != 0 && !b.longFired) {
    if (millis() - b.downMs >= BTN_LONGPRESS_MS) {
      b.longFired = true;
      b.longEvent = true;   // Wichtig: dadurch wird spaeter KEIN Short-Press erzeugt
    }
  }
}

/**
 * @brief "Consume" eines Event-Flags: gibt aktuellen Wert zurueck und setzt Flag auf false.
 */
static bool take(bool &flag) {
  bool v = flag;
  flag = false;
  return v;
}

void initNavButtons() {
  initOne(leftBtn,  BTN_LEFT);
  initOne(rightBtn, BTN_RIGHT);
}

void updateNavButtons() {
  updateOne(leftBtn,  BTN_LEFT);
  updateOne(rightBtn, BTN_RIGHT);
}

bool getLeftPressed()       { return take(leftBtn.shortEvent); }
bool getRightPressed()      { return take(rightBtn.shortEvent); }

bool getLeftLongPressed()   { return take(leftBtn.longEvent); }
bool getRightLongPressed()  { return take(rightBtn.longEvent); }

bool isLeftDown()           { return (leftBtn.lastStable == LOW); }
bool isRightDown()          { return (rightBtn.lastStable == LOW); }
