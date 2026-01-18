#include <radio_config.h>

// Netzwerk
const IPAddress RADIO_LOCAL_IP(192, 168, 52, 10);
const IPAddress RADIO_GATEWAY (192, 168, 52, 1);
const IPAddress RADIO_SUBNET  (255, 255, 255, 0);

// Radio Ziel
const char* RADIO_IP = "192.168.52.34";
const uint16_t RADIO_PORT = 4655;

// Befehle
const char* CMD_RADIO_ON  = "M:REMOTE SENTER2,0";
const char* CMD_RADIO_OFF = "M:REMOTE SENTER0";

// Frequenz setzen: "M:FF SRF3" + <Hz>
const char* CMD_SET_FRQ_FMT = "M:FF SRF%ld";

// Modulation: aktuell nur A3E
const char* const CMD_SET_MOD_LIST[] = {
  "M:FF SMD9"  // A3E
};
const int CMD_SET_MOD_COUNT = sizeof(CMD_SET_MOD_LIST) / sizeof(CMD_SET_MOD_LIST[0]);
