#include <Arduino.h>

#include <TFTDisplay.h>
#include <RotaryEncoder.h>
#include <NavButtons.h>
#include <GUI.h>

#include <RadioTCP.h>

void setup() {
  Serial.begin(115200);
  delay(200);

  initDisplay();
  runBit();

  initRotaryEncoder();
  initNavButtons();

  // Netzwerk
  radioInit();

  // UI
  guiInit();
  guiSetRadioOn(radioIsRadioOn());
}

void loop() {
  guiUpdate();
  radioUpdate();

  // 1) ON Toggle aus GUI (Footer-Fokus ON + Encoder Short)
  if (guiConsumeOnToggleRequested()) {
    if (!radioIsRadioOn()) {
      bool ok = radioConnect();
      guiSetRadioOn(ok && radioIsRadioOn());
    } else {
      radioDisconnect();
      guiSetRadioOn(false);
    }
  }

  // 2) Save (Encoder Long im Edit) -> ans Radio senden, wenn Radio ON
  GuiScreen saved;
  if (guiConsumeSaveRequested(saved)) {
    if (!radioIsRadioOn()) {
      // Nur lokal gespeichert (Toast), nichts senden
      return;
    }

    if (saved == GUI_FRQ) {
      radioSetFrequencyHz(guiGetFrequencyHz());
    } else if (saved == GUI_MOD) {
      radioSetModulationIndex(guiGetModIndex());
    } else {
      // PWR: wie gewünscht noch überspringen
    }
  }
}
