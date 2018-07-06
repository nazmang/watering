#pragma once

#ifdef DEBUG
#define         DEBUG_PRINT(x)    Serial.print(x)
#define         DEBUG_PRINTLN(x)  Serial.println(x)
#else
#define         DEBUG_PRINT(x)
#define         DEBUG_PRINTLN(x)
#endif

#define B_1 0
#define B_2 9
#define B_3 10
#define B_4 14
#define L_1 12
#define L_2 5
#define L_3 4
#define L_4 15
#define LED 13
#define RAIN_SENSOR 16
#define HOST_PREFIX  "Sonoff_%s"
#define HEADER       "\n\n--------------  KmanSonoff_v1.00mc  --------------"
#define VER          "ksmc_v1.00"
#define STRUCT_CHAR_ARRAY_SIZE 24   // size of the arrays for MQTT username, password, etc.

typedef struct {
	char            mqttUser[STRUCT_CHAR_ARRAY_SIZE] = "";//{0};
	char            mqttPassword[STRUCT_CHAR_ARRAY_SIZE] = "";//{0};
	char            mqttServer[STRUCT_CHAR_ARRAY_SIZE] = "";//{0};
	char            mqttPort[6] = "";//{0};
} Settings;

bool requestRestart = false;
bool OTAupdate = false;
char ESP_CHIP_ID[8];
char UID[16];
long rssi;
unsigned long TTasks;
Settings settings;
WiFiUDP ntpUDP;
IPAddress timeServerIP;
String ntpServerName = "pool.ntp.org";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
unsigned int localPort = 2390;      // local port to listen for UDP packets
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets


// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
	DEBUG_PRINTLN("sending NTP packet...");
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
							 // 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	ntpUDP.beginPacket(address, 123); //NTP requests are to port 123
	ntpUDP.write(packetBuffer, NTP_PACKET_SIZE);
	ntpUDP.endPacket();
}

time_t getNTPtime() {
	DEBUG_PRINTLN("Starting UDP");
	ntpUDP.begin(localPort);
	DEBUG_PRINT("Local port: ");
	DEBUG_PRINTLN(ntpUDP.localPort());
	DEBUG_PRINTLN("waiting for sync");

	while (ntpUDP.parsePacket() > 0); // discard any previously received packets
	DEBUG_PRINTLN("Transmit NTP Request to " + timeServerIP.toString());
	sendNTPpacket(timeServerIP);
	uint32_t beginWait = millis();
	while (millis() - beginWait < 1500) {
		int size = ntpUDP.parsePacket();
		if (size >= NTP_PACKET_SIZE) {
			DEBUG_PRINTLN("Receive NTP Response");
			ntpUDP.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
			unsigned long secsSince1900;
			// convert four bytes starting at location 40 to a long integer
			secsSince1900 = (unsigned long)packetBuffer[40] << 24;
			secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
			secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
			secsSince1900 |= (unsigned long)packetBuffer[43];
			return secsSince1900 - 2208988800UL + NTP_TIMEZONE * SECS_PER_HOUR;
		}
	}
	DEBUG_PRINTLN("No NTP Response :-(");
	return 0; // return 0 if unable to get the time	
}

// flag for saving data
bool shouldSaveConfig = false;
// callback notifying us of the need to save config
void saveConfigCallback() {
	shouldSaveConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager) {
	//ticker.attach(0.2, tick);
}

Ticker sensor_timer;
bool isRain = false;