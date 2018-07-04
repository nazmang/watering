/*
  Copyright (c) 2017 @KmanOz
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  **** Use this Firmware for: Sonoff 4CH, 4CH Pro & T1 (1, 2 and 3 Channel) ****
  **** Make sure to select "Generic ESP8285 Module" from the BOARD menu in TOOLS ****
  **** Flash Size "1M (64K SPIFFS)" ****

  ===============================================================================================
     ATTENTION !!!!!! DO NOT CHANGE ANYTHING IN THIS SECTION. UPDATE YOUR DETAILS IN CONFIG.H
  ===============================================================================================
*/

#include <ESP8266WebServerSecure.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <TimeLib.h>
#include <Time.h>
#include "config_mc.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <TimeAlarms.h>
//#include <DNSServer.h>       




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
	Serial.println("sending NTP packet...");
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
	Serial.println("Starting UDP");
	ntpUDP.begin(localPort);
	Serial.print("Local port: ");
	Serial.println(ntpUDP.localPort());
	Serial.println("waiting for sync");

	while (ntpUDP.parsePacket() > 0); // discard any previously received packets
	Serial.println("Transmit NTP Request to "+ timeServerIP.toString());
	sendNTPpacket(timeServerIP);
	uint32_t beginWait = millis();
	while (millis() - beginWait < 1500) {
		int size = ntpUDP.parsePacket();
		if (size >= NTP_PACKET_SIZE) {
			Serial.println("Receive NTP Response");
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
	Serial.println("No NTP Response :-(");
	return 0; // return 0 if unable to get the time	
}



Ticker sensor_timer;
bool isRain = false;

#ifdef CH_1
  bool sendStatus1 = false;
  int  SS1;
  unsigned long count1 = 0;
  Ticker btn_timer1;
  void channel1_on() { if (!isRain) digitalWrite(L_1, HIGH); else Serial.println("It's raining now! Can't turn Relay 1 ON "); sendStatus1 = true; }
  void channel1_off() { digitalWrite(L_1, LOW); sendStatus1 = true; }
#endif
#ifdef CH_2
  bool sendStatus2 = false;
  int  SS2;
  unsigned long count2 = 0;
  Ticker btn_timer2;
  void channel2_on() { if (!isRain) digitalWrite(L_2, HIGH); else Serial.println("It's raining now! Can't turn Relay 2 ON "); sendStatus2 = true; }
  void channel2_off() { digitalWrite(L_2, LOW); sendStatus2 = true; }
#endif
#ifdef CH_3
  bool sendStatus3 = false;
  int  SS3;
  unsigned long count3 = 0;
  Ticker btn_timer3;
  void channel3_on() { if (!isRain) digitalWrite(L_3, HIGH); else Serial.println("It's raining now! Can't turn Relay 3 ON "); sendStatus3 = true; }
  void channel3_off() { digitalWrite(L_3, LOW); sendStatus3 = true; }
#endif
#ifdef CH_4
  bool sendStatus4 = false;
  int  SS4;
  unsigned long count4 = 0;
  Ticker btn_timer4;
  void channel4_on() { if (!isRain) digitalWrite(L_4, HIGH); else Serial.println("It's raining now! Can't turn Relay 4 ON "); sendStatus4 = true; }
  void channel4_off() { digitalWrite(L_4, LOW); sendStatus4 = true; }
#endif
extern "C" { 
  #include "user_interface.h" 
}
#ifdef MQTT_TLS
WiFiClientSecure  wifiClient;
#else
WiFiClient        wifiClient;
#endif
PubSubClient mqttClient(wifiClient, MQTT_SERVER, MQTT_PORT);




void callback(const MQTT::Publish& pub) {
  if (pub.payload_string() == "stat") {
  }
  #ifdef CH_1
    else if (pub.payload_string() == "1on") {
      digitalWrite(L_1, HIGH);
      sendStatus1 = true;
    }
    else if (pub.payload_string() == "1off") {
      digitalWrite(L_1, LOW);
      sendStatus1 = true;
    }
  #endif
  #ifdef CH_2
    else if (pub.payload_string() == "2on") {
      digitalWrite(L_2, HIGH);
      sendStatus2 = true;
    }
    else if (pub.payload_string() == "2off") {
      digitalWrite(L_2, LOW);
      sendStatus2 = true;
    }
  #endif
  #ifdef CH_3
    else if (pub.payload_string() == "3on") {
      digitalWrite(L_3, HIGH);
      sendStatus3 = true;
    }
    else if (pub.payload_string() == "3off") {
      digitalWrite(L_3, LOW);
      sendStatus3 = true;
    }
  #endif
  #ifdef CH_4
    else if (pub.payload_string() == "4on") {
      digitalWrite(L_4, HIGH);
      sendStatus4 = true;
    }
    else if (pub.payload_string() == "4off") {
      digitalWrite(L_4, LOW);
      sendStatus4 = true;
    }
  #endif
  else if (pub.payload_string() == "reset") {
    requestRestart = true;
  }
}

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  
 
  Serial.begin(115200);
  sprintf(ESP_CHIP_ID, "%06X", ESP.getChipId());
  sprintf(UID, HOST_PREFIX, ESP_CHIP_ID);

#ifdef TLS
  WiFiManagerParameter custom_text("<p>MQTT username, password and broker port</p>");
  WiFiManagerParameter custom_mqtt_server("mqtt-server", "MQTT Broker IP", "m21.cloudmqtt.com", STRUCT_CHAR_ARRAY_SIZE, "disabled");
#else
  WiFiManagerParameter custom_text("<p>MQTT username, password, broker IP address and broker port</p>");
  WiFiManagerParameter custom_mqtt_server("mqtt-server", "MQTT Broker IP", settings.mqttServer, STRUCT_CHAR_ARRAY_SIZE);
#endif
  WiFiManagerParameter custom_mqtt_user("mqtt-user", "MQTT User", settings.mqttUser, STRUCT_CHAR_ARRAY_SIZE);
  WiFiManagerParameter custom_mqtt_password("mqtt-password", "MQTT Password", settings.mqttPassword, STRUCT_CHAR_ARRAY_SIZE, "type = \"password\"");
  WiFiManagerParameter custom_mqtt_port("mqtt-port", "MQTT Broker Port", settings.mqttPort, 6);

  WiFiManager wifiManager;

  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(180);
  // set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // Rain sensor
  pinMode(RAIN_SENSOR, INPUT);
  sensor_timer.attach(300, detectRain);

  EEPROM.begin(8);
  #ifdef CH_1
    pinMode(B_1, INPUT);
    pinMode(L_1, OUTPUT);
    digitalWrite(L_1, LOW);
    SS1 = EEPROM.read(0);
    if (rememberRelayState1 && SS1 == 1) {
      digitalWrite(L_1, HIGH);
    }
	
  //  btn_timer1.attach(0.05, button1);
  #endif
  #ifdef CH_2
    pinMode(B_2, INPUT);
    pinMode(L_2, OUTPUT);
    digitalWrite(L_2, LOW);
    SS2 = EEPROM.read(1);
    if (rememberRelayState2 && SS2 == 1) {
      digitalWrite(L_2, HIGH);
    }
	
   // btn_timer2.attach(0.05, button2);
  #endif
  #ifdef CH_3
    pinMode(B_3, INPUT);
    pinMode(L_3, OUTPUT);
    digitalWrite(L_3, LOW);
    SS3 = EEPROM.read(2);
    if (rememberRelayState3 && SS3 == 1) {
      digitalWrite(L_3, HIGH);
    }
	
   // btn_timer3.attach(0.05, button3);
  #endif
  #ifdef CH_4
    pinMode(B_4, INPUT);
    pinMode(L_4, OUTPUT);
    digitalWrite(L_4, LOW);
    SS4 = EEPROM.read(3);
    if (rememberRelayState4 && SS4 == 1) {
      digitalWrite(L_4, HIGH);
    }
	
    //btn_timer4.attach(0.05, button4);
  #endif
  mqttClient.set_callback(callback);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(UID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  ArduinoOTA.setHostname(UID);
  ArduinoOTA.onStart([]() {
    OTAupdate = true;
    blinkLED(LED, 400, 2);
    digitalWrite(LED, HIGH);
    Serial.println("OTA Update Initiated . . .");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Update Ended . . .s");
    OTAupdate = false;
    requestRestart = true;
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    digitalWrite(LED, LOW);
    delay(5);
    digitalWrite(LED, HIGH);
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    blinkLED(LED, 40, 2);
    OTAupdate = false;
    Serial.printf("OTA Error [%u] ", error);
    if (error == OTA_AUTH_ERROR) Serial.println(". . . . . . . . . . . . . . . Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println(". . . . . . . . . . . . . . . Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println(". . . . . . . . . . . . . . . Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println(". . . . . . . . . . . . . . . Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println(". . . . . . . . . . . . . . . End Failed");
  });
  ArduinoOTA.begin();
  Serial.println(HEADER);
  Serial.print("\nUnit ID: ");
  Serial.print(UID);
  Serial.print("\nConnecting to "); Serial.print(WIFI_SSID); Serial.print(" Wifi"); 
  
  
  while ((WiFi.status() != WL_CONNECTED) && kRetries --) {
    delay(500);
    Serial.print(" .");
  }
  if (WiFi.status() == WL_CONNECTED) {  
    Serial.println(" DONE");
    Serial.print("IP Address is: "); Serial.println(WiFi.localIP());
	Serial.print("Connecting to ");Serial.print(MQTT_SERVER);Serial.print(" Broker . .");
    delay(500);
    while (!mqttClient.connect(MQTT::Connect(UID).set_keepalive(90).set_auth(MQTT_USER, MQTT_PASS)) && kRetries --) {
      Serial.print(" .");
      delay(1000);
    }
	
    if(mqttClient.connected()) {
      Serial.println(" DONE");
      Serial.println("\n---------------------  Logs  ---------------------");
      Serial.println();
      mqttClient.subscribe(MQTT_TOPIC);
      blinkLED(LED, 40, 8);
      digitalWrite(LED, LOW);	 
	  //	  
	  Serial.println("Setting time via NTP ...");
#ifdef NTP_SERVER1
	  ntpServerName = NTP_SERVER1;
#endif // NTP_SERVER1

	  WiFi.hostByName(ntpServerName.c_str(), timeServerIP);
	  setTime(getNTPtime());
	  if (timeStatus() == timeSet) {
		  Serial.print("Time is set. Current timestamp is ");
		  Serial.println(now());
		  setSyncProvider(getNTPtime);
	  }
    }
    else {
      Serial.println(" FAILED!");
      Serial.println("\n--------------------------------------------------");
      Serial.println();
    }
  }
  else {
    Serial.println(" WiFi FAILED!");
    Serial.println("\n--------------------------------------------------");
    Serial.println();
	requestRestart = true;
  }
  if (timeStatus() == timeSet) {
	  
#ifdef CH_1
	  Alarm.alarmRepeat(7, 20, 0, channel1_on);
	  Alarm.alarmRepeat(7, 40, 0, channel1_off);
	  Alarm.alarmRepeat(22, 52, 0, channel1_on);
	  Alarm.alarmRepeat(23, 00, 0, channel1_off);
#endif
#ifdef CH_2
	  Alarm.alarmRepeat(7, 40, 0, channel2_on);
	  Alarm.alarmRepeat(8, 0, 0, channel2_off);
	  Alarm.alarmRepeat(23, 10, 0, channel2_on);
	  Alarm.alarmRepeat(23, 20, 0, channel2_off);
#endif
#ifdef CH_3
	  Alarm.alarmRepeat(8, 0, 0, channel3_on);
	  Alarm.alarmRepeat(8, 20, 0, channel3_off);
	  Alarm.alarmRepeat(23, 20, 0, channel3_on);
	  Alarm.alarmRepeat(23, 30, 0, channel3_off);
#endif
#ifdef CH_4
	  Alarm.alarmRepeat(8, 20, 0, channel3_on);
	  Alarm.alarmRepeat(8, 40, 0, channel3_off);
	  Alarm.alarmRepeat(23, 30, 0, channel3_on);
	  Alarm.alarmRepeat(23, 40, 0, channel3_off);
#endif
  }
  else
  {
	  Serial.println("Time is not set! Can't initialize timers.");
  }
}

void loop() { 
  ArduinoOTA.handle();
  Alarm.delay(10);
  
  if (OTAupdate == false) { 
    mqttClient.loop();
    timedTasks();
    checkStatus();	
  }
}

void digitalClockDisplay()
{
	// digital clock display of the time
	Serial.print(hour());
	printDigits(minute());
	printDigits(second());
	Serial.println();
}

void printDigits(int digits)
{
	Serial.print(":");
	if (digits < 10)
		Serial.print('0');
	Serial.print(digits);
}

void blinkLED(int pin, int duration, int n) {             
  for(int i=0; i<n; i++)  {  
    digitalWrite(pin, HIGH);        
    delay(duration);
    digitalWrite(pin, LOW);
    delay(duration);
  }
}

void detectRain() {
	isRain = digitalRead(RAIN_SENSOR);
}

#ifdef CH_1
  void button1() {
    if (!digitalRead(B_1)) {
      count1++;
    } 
    else {
      if (count1 > 1 && count1 <= 40) {   
        digitalWrite(L_1, !digitalRead(L_1));
       sendStatus1 = true;
      } 
      else if (count1 >40){
        Serial.println("\n\nSonoff Rebooting . . . . . . . . Please Wait"); 
        requestRestart = true;
      } 
      count1=0;
    }
  }
#endif
#ifdef CH_2
  void button2() {
    if (!digitalRead(B_2)) {
      count2++;
    } 
    else {
      if (count2 > 1 && count2 <= 40) {   
        digitalWrite(L_2, !digitalRead(L_2));
        sendStatus2 = true;
      } 
      count2=0;
    }
  }
#endif
#ifdef CH_3
  void button3() {
    if (!digitalRead(B_3)) {
     count3++;
    } 
    else {
      if (count3 > 1 && count3 <= 40) {   
        digitalWrite(L_3, !digitalRead(L_3));
        sendStatus3 = true;
      } 
      count3=0;
    }
  }
#endif
#ifdef CH_4
  void button4() {
    if (!digitalRead(B_4)) {
      count4++;
    } 
    else {
      if (count4 > 1 && count4 <= 40) {   
        digitalWrite(L_4, !digitalRead(L_4));
        sendStatus4 = true;
      } 
      count4=0;
    }
  }
#endif

void checkConnection() {
  if (WiFi.status() == WL_CONNECTED)  {
    if (mqttClient.connected()) {
      Serial.println("mqtt broker connection . . . . . . . . . . OK");
    } 
    else {
      Serial.println("mqtt broker connection . . . . . . . . . . LOST");
	  Serial.print("Reconnecting ");
	  while (!mqttClient.connect(MQTT::Connect(UID).set_keepalive(90).set_auth(MQTT_USER, MQTT_PASS)) && kRetries--) {
		  Serial.print(" .");
		  delay(1000);
	  }
      requestRestart = true;
    }
  }
  else { 
    Serial.println("WiFi connection . . . . . . . . . . LOST");
    requestRestart = true;
  }
}

void checkStatus() {
  #ifdef CH_1
    if (sendStatus1) {
      if(digitalRead(L_1) == LOW)  {
        if (rememberRelayState1) {
          EEPROM.write(0, 0);
          EEPROM.commit();
        }
        if (kRetain == 0) {
          mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "1off").set_qos(QOS));
        } else {
          mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "1off").set_retain().set_qos(QOS));
        }
        Serial.println("Relay 1 . . . . . . . . . . . . . . . . . . OFF");
      } else {
        if (rememberRelayState1) {
          EEPROM.write(0, 1);
          EEPROM.commit();
        }
      if (kRetain == 0) {
        mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "1on").set_qos(QOS));
      } else {
        mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "1on").set_retain().set_qos(QOS));
      }
      Serial.println("Relay 1 . . . . . . . . . . . . . . . . . . ON");
      }
      sendStatus1 = false;
    }
  #endif
  #ifdef CH_2
    if (sendStatus2) {
      if(digitalRead(L_2) == LOW)  {
        if (rememberRelayState2) {
          EEPROM.write(1, 0);
          EEPROM.commit();
        }
        if (kRetain == 0) {
          mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "2off").set_qos(QOS));
        } else {
          mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "2off").set_retain().set_qos(QOS));
        }
        Serial.println("Relay 2 . . . . . . . . . . . . . . . . . . OFF");
      } else {
        if (rememberRelayState2) {
          EEPROM.write(1, 1);
          EEPROM.commit();
        }
      if (kRetain == 0) {
        mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "2on").set_retain().set_qos(QOS));
      } else {
        mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "2on").set_qos(QOS));
      }
      Serial.println("Relay 2 . . . . . . . . . . . . . . . . . . ON");
      }
      sendStatus2 = false;
    }
  #endif
  #ifdef CH_3
    if (sendStatus3) {
      if(digitalRead(L_3) == LOW)  {
        if (rememberRelayState3) {
          EEPROM.write(2, 0);
          EEPROM.commit();
        }
        if (kRetain == 0) {
          mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "3off").set_qos(QOS));
        } else {
          mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "3off").set_retain().set_qos(QOS));
        }
        Serial.println("Relay 3 . . . . . . . . . . . . . . . . . . OFF");
      } else {
        if (rememberRelayState3) {
          EEPROM.write(2, 1);
          EEPROM.commit();
        }
      if (kRetain == 0) {
        mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "3on").set_qos(QOS));
      } else {
        mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "3on").set_retain().set_qos(QOS));
      }
      Serial.println("Relay 3 . . . . . . . . . . . . . . . . . . ON");
      }
      sendStatus3 = false;
    }
  #endif
  #ifdef CH_4
    if (sendStatus4) {
      if(digitalRead(L_4) == LOW)  {
        if (rememberRelayState4) {
          EEPROM.write(3, 0);
          EEPROM.commit();
        }
        if (kRetain == 0) {
          mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "4off").set_qos(QOS));
        } else {
          mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "4off").set_retain().set_qos(QOS));
        }
        Serial.println("Relay 4 . . . . . . . . . . . . . . . . . . OFF");
      } else {
        if (rememberRelayState4) {
          EEPROM.write(3, 1);
          EEPROM.commit();
        }
      if (kRetain == 0) {
        mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "4on").set_qos(QOS));
      } else {
        mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "4on").set_retain().set_qos(QOS));
      }
      Serial.println("Relay 4 . . . . . . . . . . . . . . . . . . ON");
      }
      sendStatus4 = false;
    }
  #endif
  if (requestRestart) {
    blinkLED(LED, 400, 4);
    ESP.restart();
  }
}

void doReport() {
  rssi = WiFi.RSSI();
  char message_buff[140];
  String pubString = "{\"Time\": "+String(now())+", "+"\"UID\": \""+String(UID)+"\", "+"\"WiFi RSSI\": \""+ String(rssi)+"dBM\""+", "+"\"Topic\": \""+String(MQTT_TOPIC)+"\", "+"\"Ver\": \""+String(VER)+"\"}";
  pubString.toCharArray(message_buff, pubString.length()+1);
  if (kRetain == 0) {
    mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/debug", message_buff).set_qos(QOS));
    mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/heartbeat", "OK").set_qos(QOS));
	mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/rain", String(isRain)).set_qos(QOS));
  } else {
    mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/debug", message_buff).set_retain().set_qos(QOS));
    mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/heartbeat", "OK").set_retain().set_qos(QOS));
	mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/rain", String(isRain)).set_qos(QOS));
  }
}

void timedTasks() {
  if ((millis() > TTasks + (kUpdFreq*30000)) || (millis() < TTasks)) { 
    TTasks = millis();
    doReport();
    checkConnection();
	digitalClockDisplay();
	if (timeStatus() == timeNeedsSync) {
		Serial.println("Time needs synchronization. Updating..."); 
		WiFi.hostByName(ntpServerName.c_str(), timeServerIP);
		getNTPtime();
	}
  }
}


