#pragma once
#include <Arduino.h>
#include <stdint.h>

void initNavButtons();
void updateNavButtons();

// Short Press (nur wenn KEIN Long-Press)
bool getLeftPressed();
bool getRightPressed();

// Long Press (einmalig nach Haltezeit)
bool getLeftLongPressed();
bool getRightLongPressed();

// Optional: aktueller stabiler Zustand
bool isLeftDown();
bool isRightDown();
