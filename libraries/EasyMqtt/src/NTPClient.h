#pragma once

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define SEVENZYYEARS 2208988800UL
#define NTP_PACKET_SIZE 48
#define NTP_DEFAULT_LOCAL_PORT 1337

class NTPClient {
  private:
    WiFiUDP udp;
    long localEpoc = 0;
    long localMillisAtUpdate;
    const char* ntpServerName = "pool.ntp.org";
    const int httpServerPort = NTP_DEFAULT_LOCAL_PORT;
    byte packetBuffer[NTP_PACKET_SIZE];

    void sendNTPPacket();

  public:
    NTPClient();
    void update();
    long getTime();
    long getTime(long millis);
};
