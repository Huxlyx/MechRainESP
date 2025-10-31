#include "Discovery.h"

ServerInfo serverInfo;
WiFiUDP udp;
const int UDP_PORT = 5000;

void sendDiscovery() {
	IPAddress broadcastIp(255,255,255,255);
	udp.beginPacket(broadcastIp, UDP_PORT);
	udp.print("MECH-RAIN-HELLO");
	udp.endPacket();
	Serial.println("UDP Discovery sent");
}

/* "MECH-RAIN-SERVER:IP=192.168.x.x;PORT=nnnn" */
bool parseServerInfo() {
	if (udp.parsePacket() == 0) {
		return 0;
	}
	
	char buffer[255];
	
	int len = udp.read(buffer, sizeof(buffer) - 1);
	buffer[len] = 0;
			
	String s = String(buffer);
	Serial.print("Received message: ");
	Serial.println(s);
	if (!s.startsWith("MECH-RAIN-SERVER:")) {
		return false;
	}

	int ipPos = s.indexOf("IP=");
	int portPos = s.indexOf("PORT=");

	if (ipPos == -1 || portPos == -1) {
		return false;
	}

	String ipStr = s.substring(ipPos + 3, s.indexOf(";", ipPos));
	String portStr = s.substring(portPos + 5);

	if (!serverInfo.ip.fromString(ipStr)) {
		Serial.println("Received invalid IP");
		return false;
	}	

	serverInfo.port = portStr.toInt();
	return serverInfo.port > 0;
}