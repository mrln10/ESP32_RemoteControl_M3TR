// lib/RotaryEncoder/RotaryEncoder.h

#pragma once
#include <Arduino.h>
#include <stdint.h>

// Initialisieren (Pins aus config.h)
void initRotaryEncoder();

// Muss zyklisch in loop() aufgerufen werden
void updateRotaryEncoder();

// Dreh-Event seit letztem Abruf (typisch -1/0/+1 pro Rastung)
int32_t getEncoderDelta();

// Button-Events
bool getButtonPressed();        // SHORT press (nur wenn KEIN Long-Press)
bool getButtonLongPressed();    // LONG press (einmalig, sobald Zeit erreicht)
bool isButtonDown();            // aktueller stabiler Zustand

// Debug Helpers (optional)
int readEncoderCLK();
int readEncoderDT();
int readEncoderSW();
