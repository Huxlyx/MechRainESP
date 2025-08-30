// Discovery.h
#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <WiFi.h>
#include <WiFiUdp.h>

struct ServerInfo {
  IPAddress ip;
  int port;
};

extern ServerInfo serverInfo;
extern WiFiUDP udp;
extern const int UDP_PORT;

void sendDiscovery();
bool parseServerInfo();

#endif  // DISCOVERY_H