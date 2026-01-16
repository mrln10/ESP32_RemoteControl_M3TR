// src/gui_config.cpp
//
// Zentraler Ort für "Definitionen" der GUI-Konstanten.
// Hintergrund:
// - gui_config.h enthält extern-Deklarationen (C++11-kompatibel, keine Mehrfachdefinitionen)
// - Diese .cpp liefert genau eine Definition je Symbol.
// Vorteil:
// - Keine C++17-inline-Variablen nötig
// - Konfigurationswerte sind trotzdem zentral gepflegt

#include <gui_config.h>

// ---------------------------
// Modulationsarten (Liste)
// ---------------------------
// Hinweis: Reihenfolge entspricht der Auswahlreihenfolge im UI.
const char* const GUI_MOD_LIST[] = {
  "AM", "FM", "USB", "LSB", "CW", "DIGI"
};
const int GUI_MOD_COUNT = sizeof(GUI_MOD_LIST) / sizeof(GUI_MOD_LIST[0]);

// ---------------------------
// Power-Modi (Liste)
// ---------------------------
const char* const GUI_PWR_LIST[] = {
  "LOW", "MED", "HIGH"
};
const int GUI_PWR_COUNT = sizeof(GUI_PWR_LIST) / sizeof(GUI_PWR_LIST[0]);

// ---------------------------
// Defaults / Theme / Limits
// ---------------------------
// Diese Objekte nutzen die Default-Initialwerte aus gui_config.h
// und werden von GUI.cpp direkt verwendet.
const GuiDefaults GUI_DEFAULTS{};
const GuiTheme GUI_THEME{};
const GuiConstraints GUI_LIMITS{};
