#pragma once
#include <stdint.h>
#include <IPAddress.h>

/*
  RadioTCP Konfiguration (C++11 kompatibel via extern + .cpp)
  ----------------------------------------------------------
  Protokoll: LF + CMD + CR

  Befehle (vom Nutzer vorgegeben):
    - Radio an:  M:REMOTE SENTER2,0
    - Radio aus: M:REMOTE SENTER0
    - Frequenz:  M:FF SRF3<Hz>   (Beispiel: 30100000 für 30.1 MHz)
    - Modulation A3E: M:FF SMD9
*/

// Netzwerk (statische IP)
extern const IPAddress RADIO_LOCAL_IP;
extern const IPAddress RADIO_GATEWAY;
extern const IPAddress RADIO_SUBNET;

// Radio Ziel
extern const char* RADIO_IP;
extern const uint16_t RADIO_PORT;

// Befehle
extern const char* CMD_RADIO_ON;
extern const char* CMD_RADIO_OFF;

// Format: "M:FF SRF3%ld" -> %ld = freq_hz (z.B. 30100000)
extern const char* CMD_SET_FRQ_FMT;

// Modulations-Kommandos als Liste (Index = GUI Mod Index)
extern const char* const CMD_SET_MOD_LIST[];
extern const int CMD_SET_MOD_COUNT;
