#include "RadioTCP.h"

#include <ETH.h>
#include <WiFiClient.h>
#include <radio_config.h>

static WiFiClient client;
static bool ethReady = false;
static bool radioOn  = false;

static bool ensureTcpConnected();
static bool sendCommand(const char* cmd);

bool radioInit() {
  ethReady = false;
  radioOn  = false;

  if (!ETH.begin()) {
    Serial.println("[RadioTCP] ETH.begin() fehlgeschlagen!");
    return false;
  }
  Serial.println("[RadioTCP] ETH.begin() ok");

  ETH.config(RADIO_LOCAL_IP, RADIO_GATEWAY, RADIO_SUBNET);

  Serial.print("[RadioTCP] Warte auf Ethernet-Link/IP");
  uint32_t t0 = millis();
  while (millis() - t0 < 8000) {
    if (ETH.linkUp() && ETH.localIP()[0] != 0) {
      ethReady = true;
      break;
    }
    Serial.print(".");
    delay(250);
  }
  Serial.println();

  if (!ethReady) {
    Serial.println("[RadioTCP] Ethernet nicht bereit (Link/IP).");
    return false;
  }

  Serial.print("[RadioTCP] ETH Link UP, IP: ");
  Serial.println(ETH.localIP());
  return true;
}

void radioUpdate() {
  if (!ethReady) return;

  // Antwortdaten (Debug)
  while (client.connected() && client.available()) {
    int c = client.read();
    if (c >= 0) Serial.write((char)c);
  }
}

bool radioIsEthReady()     { return ethReady; }
bool radioIsTcpConnected() { return client.connected(); }
bool radioIsRadioOn()      { return radioOn; }

static bool ensureTcpConnected() {
  if (!ethReady) return false;
  if (client.connected()) return true;

  Serial.println("[RadioTCP] Verbinde TCP...");
  client.stop();
  delay(50);

  if (client.connect(RADIO_IP, RADIO_PORT)) {
    Serial.println("[RadioTCP] TCP verbunden.");
    return true;
  }

  Serial.println("[RadioTCP] TCP connect fehlgeschlagen.");
  return false;
}

static bool sendCommand(const char* cmd) {
  if (!cmd || cmd[0] == '\0') return false;
  if (!ensureTcpConnected()) return false;

  // Protokoll: LF + CMD + CR
  client.write('\n');
  client.print(cmd);
  client.write('\r');

  Serial.print("[RadioTCP] Gesendet: ");
  Serial.println(cmd);
  return true;
}

bool radioConnect() {
  if (!ethReady) return false;
  if (!ensureTcpConnected()) return false;

  if (!sendCommand(CMD_RADIO_ON)) return false;
  radioOn = true;
  return true;
}

bool radioDisconnect() {
  if (!ethReady) return false;

  if (client.connected()) {
    sendCommand(CMD_RADIO_OFF);
    delay(20);
  }
  client.stop();
  radioOn = false;

  Serial.println("[RadioTCP] Radio OFF / TCP getrennt.");
  return true;
}

bool radioSendRaw(const char* cmd) {
  return sendCommand(cmd);
}

bool radioSetFrequencyHz(int32_t freq_hz) {
  char buf[64];
  snprintf(buf, sizeof(buf), CMD_SET_FRQ_FMT, (long)freq_hz);
  return sendCommand(buf);
}

bool radioSetModulationIndex(int mod_index) {
  if (mod_index < 0 || mod_index >= CMD_SET_MOD_COUNT) return false;
  return sendCommand(CMD_SET_MOD_LIST[mod_index]);
}

