#pragma once
#include <Arduino.h>
#include <stdint.h>

/*
  RadioTCP Modul
  --------------
  - Initialisiert Ethernet (ETH.begin, static IP)
  - Baut TCP Verbindung zum Radio auf
  - Sendet Commands als: LF + CMD + CR
  - High-Level: connect/disconnect, setFrequencyHz, setModulationIndex
*/

bool radioInit();
void radioUpdate();

bool radioIsEthReady();
bool radioIsTcpConnected();
bool radioIsRadioOn();

bool radioConnect();     // TCP connect + CMD_RADIO_ON
bool radioDisconnect();  // CMD_RADIO_OFF + TCP stop

bool radioSendRaw(const char* cmd);

bool radioSetFrequencyHz(int32_t freq_hz);
bool radioSetModulationIndex(int mod_index);
