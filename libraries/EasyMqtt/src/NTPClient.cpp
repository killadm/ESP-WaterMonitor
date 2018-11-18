#include "NTPClient.h"
#include <WiFiUdp.h>

// https://playground.arduino.cc/Code/NTPclient

NTPClient::NTPClient() {
  WiFiUDP ntp();
  this->udp.begin(123);
}

void NTPClient::update() {
  this->sendNTPPacket();
  byte timeout = 0;
  int cb = 0;
  do {
    delay ( 10 );
    cb = this->udp.parsePacket();
    if (timeout > 100) return;
    timeout++;
  } while (cb == 0);

  localMillisAtUpdate = millis() - (10 * ((timeout + 1) / 2));
  this->udp.read(this->packetBuffer, NTP_PACKET_SIZE);

  unsigned long highWord = word(this->packetBuffer[40], this->packetBuffer[41]);
  unsigned long lowWord = word(this->packetBuffer[42], this->packetBuffer[43]);

  unsigned long secsSince1900 = highWord << 16 | lowWord;

  localEpoc = secsSince1900 - SEVENZYYEARS;
}

void NTPClient::sendNTPPacket() {
  memset(this->packetBuffer, 0, NTP_PACKET_SIZE);
  this->packetBuffer[0] = 0b11100011;
  this->packetBuffer[1] = 0;
  this->packetBuffer[2] = 6;
  this->packetBuffer[3] = 0xEC;
  this->packetBuffer[12]  = 49;
  this->packetBuffer[13]  = 0x4E;
  this->packetBuffer[14]  = 49;
  this->packetBuffer[15]  = 52;

  this->udp.beginPacket(this->ntpServerName, 123);
  this->udp.write(this->packetBuffer, NTP_PACKET_SIZE);
  this->udp.endPacket();
}

long NTPClient::getTime() {
  return getTime(millis());
}

long NTPClient::getTime(long millis) {
  return localEpoc + ((millis - localMillisAtUpdate) / 1000);
}
