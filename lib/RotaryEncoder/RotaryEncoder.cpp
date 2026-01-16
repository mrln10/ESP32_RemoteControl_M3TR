// lib/RotaryEncoder/RotaryEncoder.cpp
//
// Rotary Encoder (CLK/DT) + Button (SW) als Eingabemodul.
// Features:
// - Robustere Auswertung des Drehimpulses (Quadratur-Encoder)
// - Liefert "Delta" als Schrittanzahl seit letztem Update (z.B. -1 / +1)
// - Button: Debounce + Short-Press + Long-Press
// - Garantie: Long-Press loest KEIN Short-Press aus (analog zu NavButtons)
//
// Wichtige Hinweise fuer ESP32:
// - CLK/DT sollten auf GPIOs liegen, die als Input geeignet sind.
// - Manche Encoder sind "noisy": Entprellung/Filterung ist dann entscheidend.
// - Diese Implementierung ist bewusst polling-basiert (kein Interrupt),
//   um Komplexitaet zu reduzieren und in vielen Projekten stabil zu laufen.

#include "RotaryEncoder.h"
#include <Arduino.h>
#include <config.h>

// --------------------
// Tuning-Parameter
// --------------------
static const uint32_t ENC_DEBOUNCE_US = 800;   // Mindestzeit zwischen gültigen Encoder-Flanken (Noise-Filter)
static const uint32_t BTN_DEBOUNCE_MS = 30;
static const uint32_t BTN_LONGPRESS_MS = 700;

// --------------------
// Interner Encoder-State
// --------------------
static int lastClk = HIGH;
static int lastDt  = HIGH;

static volatile int32_t deltaAccum = 0;   // akkumuliertes Delta seit letzter Abfrage

static uint32_t lastEncUs = 0;            // Zeitstempel für Noise-Filter (micros())

// Button-State (wie bei NavButtons)
struct BtnState {
  int lastStable;
  int lastReading;
  uint32_t lastChangeMs;

  bool shortEvent;
  bool longEvent;

  uint32_t downMs;
  bool longFired;
};

static BtnState btn;

/**
 * @brief Initialisiert den Buttonstate und Pin.
 */
static void initButton() {
  pinMode(ENC_SW, INPUT_PULLUP);

  int r = digitalRead(ENC_SW);
  btn.lastReading = r;
  btn.lastStable = r;
  btn.lastChangeMs = millis();

  btn.shortEvent = false;
  btn.longEvent = false;

  btn.downMs = 0;
  btn.longFired = false;
}

/**
 * @brief Pollt den Button und erzeugt Short-/Long-Events.
 *
 * Short-Press: beim Loslassen, nur wenn kein Long-Press
 * Long-Press : einmalig nach BTN_LONGPRESS_MS
 */
static void updateButton() {
  int r = digitalRead(ENC_SW);

  if (r != btn.lastReading) {
    btn.lastReading = r;
    btn.lastChangeMs = millis();
  }

  if ((millis() - btn.lastChangeMs) > BTN_DEBOUNCE_MS) {
    if (btn.lastStable != btn.lastReading) {
      btn.lastStable = btn.lastReading;

      if (btn.lastStable == LOW) {
        btn.downMs = millis();
        btn.longFired = false;
      } else {
        if (btn.downMs != 0 && !btn.longFired) {
          btn.shortEvent = true;
        }
        btn.downMs = 0;
        btn.longFired = false;
      }
    }
  }

  if (btn.lastStable == LOW && btn.downMs != 0 && !btn.longFired) {
    if (millis() - btn.downMs >= BTN_LONGPRESS_MS) {
      btn.longFired = true;
      btn.longEvent = true;
    }
  }
}

/**
 * @brief "Consume" eines Event-Flags.
 */
static bool take(bool &flag) {
  bool v = flag;
  flag = false;
  return v;
}

/**
 * @brief Encoder-Schritt berechnen (Quadratur).
 *
 * Vereinfachtes Prinzip:
 * - Wir werten eine Flanke auf CLK aus.
 * - Die Richtung ergibt sich aus dem Zustand von DT zur Flankenzeit.
 *
 * Wichtig:
 * - Manche Encoder liefern 2 oder 4 Flanken pro Rastung.
 *   Wenn dir das zu "schnell" ist, kann man die Auswertung anpassen.
 */
static void updateEncoder() {
  int clk = digitalRead(ENC_CLK);
  int dt  = digitalRead(ENC_DT);

  // Noise-Filter: nur wenn ausreichend Zeit seit letztem Ereignis vergangen ist
  uint32_t nowUs = micros();
  if ((nowUs - lastEncUs) < ENC_DEBOUNCE_US) {
    lastClk = clk;
    lastDt = dt;
    return;
  }

  // Erkennung: CLK hat eine Flanke (typisch FALLING)
  // Du kannst auch RISING nutzen; wichtig ist konsistent.
  if (clk != lastClk) {
    lastEncUs = nowUs;

    // Richtung:
    // Bei vielen Encodern gilt: wenn DT != CLK -> eine Richtung, sonst die andere.
    // Falls Richtung bei dir invertiert ist: Vorzeichen hier tauschen.
    if (dt != clk) {
      deltaAccum += 1;
    } else {
      deltaAccum -= 1;
    }
  }

  lastClk = clk;
  lastDt = dt;
}

// --------------------
// Public API
// --------------------

/**
 * @brief Initialisiert Pins für Rotary Encoder + Button.
 *
 * Erwartete Verdrahtung:
 * - ENC_CLK und ENC_DT: Encoder-Ausgänge
 * - ENC_SW: Taster gegen GND
 * - Alle Eingänge mit INPUT_PULLUP (Ruhestand HIGH, gedrückt/aktiv LOW)
 */
void initRotaryEncoder() {
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT,  INPUT_PULLUP);

  lastClk = digitalRead(ENC_CLK);
  lastDt  = digitalRead(ENC_DT);

  deltaAccum = 0;
  lastEncUs = micros();

  initButton();
}

/**
 * @brief Muss zyklisch in loop() aufgerufen werden.
 *        Pollt Encoder und Button und aktualisiert interne Events.
 */
void updateRotaryEncoder() {
  updateEncoder();
  updateButton();
}

/**
 * @brief Liefert die seit dem letzten Aufruf aufgelaufene Drehbewegung (Delta) und setzt sie zurück.
 *
 * Beispiel:
 * - Rückgabe +1: eine Rastung im Uhrzeigersinn
 * - Rückgabe -1: eine Rastung gegen den Uhrzeigersinn
 *
 * Hinweis:
 * - Wenn dein Encoder "zu fein" ist (z.B. 2 oder 4 Schritte pro Rastung),
 *   kannst du hier skalieren (z.B. nur jedes zweite Event zählen).
 */
int32_t getEncoderDelta() {
  int32_t d = deltaAccum;
  deltaAccum = 0;
  return d;
}

/**
 * @brief true genau einmal, wenn ein Short-Press erkannt wurde (und kein Long-Press).
 */
bool getButtonPressed() {
  return take(btn.shortEvent);
}

/**
 * @brief true genau einmal, wenn ein Long-Press erkannt wurde.
 *        (Long-Press unterdrückt Short-Press.)
 */
bool getButtonLongPressed() {
  return take(btn.longEvent);
}

/**
 * @brief Optional: liefert aktuellen stabilen Button-Zustand (gedrückt?)
 */
bool isButtonDown() {
  return (btn.lastStable == LOW);
}
