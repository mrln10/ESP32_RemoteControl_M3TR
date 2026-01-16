// include/gui_config.h
#pragma once
#include <stdint.h>

/*
  GUI-Konfiguration (Theme, Layout, Grenzen, Listen)
  --------------------------------------------------
  Frequenzdarstellung:
    - Anzeigeformat: "DDD.DDD MHz"
    - Interner Wert: freq_hz in Hz (int32_t)

  Auflösung:
    - 3 Nachkommastellen in MHz => 0.001 MHz = 1 kHz = 1000 Hz
    - Kleinste Stelle (Cursor ganz rechts) ändert also um 1 kHz.

  Grenzen (von dir vorgegeben):
    - 30.000 MHz .. 511.999 MHz
    - => 30'000'000 Hz .. 511'999'000 Hz

  Hinweis:
    - Wir nutzen standardmäßig CLAMP (empfohlen). Optional kann man WRAP aktivieren.
*/

struct GuiColor {
  uint8_t r, g, b;
};

struct GuiTheme {
  // Header (oben links: aktuelles Feld)
  GuiColor header_text   = {0, 255, 255};
  uint8_t  header_size   = 2;

  // Hauptwert (Frequenz / Listenwert)
  GuiColor value_text    = {255, 255, 255};
  uint8_t  value_size    = 3;   // kleiner => weniger "rausrutschen"

  // Einheit (MHz)
  GuiColor unit_text     = {180, 180, 180};
  uint8_t  unit_size     = 1;

  // Footer (Menüleiste unten)
  GuiColor footer_active = {0, 255, 255};
  GuiColor footer_idle   = {160, 160, 160};
  uint8_t  footer_size   = 1;

  // Linien / Cursor / Toast
  GuiColor line_color    = {80, 80, 80};
  GuiColor cursor_color  = {255, 255, 0};
  GuiColor toast_color   = {0, 255, 0};
};

struct GuiConstraints {
  // Frequenzgrenzen in Hz
  int32_t frq_min_hz = 30000000;    // 30.000 MHz
  int32_t frq_max_hz = 511999000;   // 511.999 MHz

  // kleinste Schrittweite in Hz (0.001 MHz = 1 kHz)
  int32_t frq_step_min_hz = 1000;

  // false = CLAMP (empfohlen), true = WRAP
  bool frq_wrap = false;

  // Toast-Dauer (Header wird durch Toast ersetzt)
  uint32_t toast_ms = 2000;

  // Layout-Zonenhöhen (in Pixeln)
  uint16_t header_h = 28;   // 0..27 (unterste Zeile ist Trennlinie)
  uint16_t footer_h = 22;   // letzte 22 Pixel
};

struct GuiDefaults {
  // 104.200 MHz => 104'200'000 Hz
  int32_t frq_start_hz = 104200000;

  int mod_index = 0;
  int pwr_index = 0;
};

// --- Listen (C++11-kompatibel: extern + Definition in src/gui_config.cpp) ---
extern const char* const GUI_MOD_LIST[];
extern const int GUI_MOD_COUNT;

extern const char* const GUI_PWR_LIST[];
extern const int GUI_PWR_COUNT;

// --- Globale Defaults/Theme/Limits (Definition in src/gui_config.cpp) ---
extern const GuiDefaults GUI_DEFAULTS;
extern const GuiTheme GUI_THEME;
extern const GuiConstraints GUI_LIMITS;
