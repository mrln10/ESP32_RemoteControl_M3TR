// src/main.cpp
//
// Entry point des Projekts.
// - Initialisiert Hardware-Module (Display, Encoder, Nav-Buttons)
// - Startet danach die GUI-State-Machine
// - Loop ruft nur guiUpdate() auf (GUI kümmert sich um Input + Rendering)

#include <Arduino.h>

#include <TFTDisplay.h>
#include <RotaryEncoder.h>
#include <NavButtons.h>
#include <GUI.h>

void setup() {
  // Optional: Debug-Ausgaben
  Serial.begin(115200);
  delay(200);

  // Display initialisieren (Rotation/Grundsetup erfolgt im TFTDisplay-Modul)
  initDisplay();

  // Optionaler Power-On-Selbsttest (IBIT/Screen-Test) – kann später entfernt/angepasst werden
  //runBit();

  // Input-Module initialisieren
  initRotaryEncoder();
  initNavButtons();

  // GUI initialisieren (zieht Theme/Limits/Listen/Defaults aus include/gui_config.h)
  guiInit();
}

void loop() {
  // GUI verarbeitet Eingaben + aktualisiert Anzeige nur bei Bedarf
  guiUpdate();
}
