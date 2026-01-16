// lib/GUI/GUI.h
#pragma once
#include <Arduino.h>
#include <stdint.h>

enum GuiScreen : uint8_t {
  GUI_FRQ = 0,
  GUI_MOD = 1,
  GUI_PWR = 2
};

// Initialisiert die GUI (zieht Theme/Limits/Listen/Defaults aus include/gui_config.h)
void guiInit();

// Muss zyklisch in loop() aufgerufen werden (Input lesen + State + partiell rendern)
void guiUpdate();

// Optional: erzwingt Full-Redraw (setzt alle Bereiche "dirty")
void guiForceRedraw();

// Status (optional)
GuiScreen guiGetScreen();
bool guiIsEditing();
