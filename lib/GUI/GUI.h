// lib/GUI/GUI.h
#pragma once
#include <Arduino.h>
#include <stdint.h>

enum GuiScreen : uint8_t {
  GUI_FRQ = 0,
  GUI_MOD = 1,
  GUI_PWR = 2
};

enum GuiFooterItem : uint8_t { FOOT_FRQ = 0, FOOT_MOD = 1, FOOT_PWR = 2, FOOT_ON = 3 };

// Initialisiert die GUI (zieht Theme/Limits/Listen/Defaults aus include/gui_config.h)
void guiInit();

// Muss zyklisch in loop() aufgerufen werden (Input lesen + State + partiell rendern)
void guiUpdate();

// Optional: erzwingt Full-Redraw (setzt alle Bereiche "dirty")
void guiForceRedraw();

// Status (optional)
GuiScreen guiGetScreen();
bool guiIsEditing();

// Radio status for footer indicator
void guiSetRadioOn(bool on);
bool guiGetRadioOn();

// Events (werden in main.cpp "consumed")
bool guiConsumeOnToggleRequested();                 // ausgelöst durch RIGHT long press
bool guiConsumeSaveRequested(GuiScreen &screen);    // ausgelöst durch Encoder long press

// Values (für Senden ans Radio)
int32_t guiGetFrequencyHz();
int guiGetModIndex();
int guiGetPwrIndex();