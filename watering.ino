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
#include "watering.h"
    

#ifdef CH_1
  bool sendStatus1 = false;
  int  SS1;
  unsigned long count1 = 0;
  Ticker btn_timer1;
  void channel1_on() { if (!isRain) digitalWrite(L_1, HIGH); else DEBUG_PRINTLN("It's raining now! Can't turn Relay 1 ON "); sendStatus1 = true; }
  void channel1_off() { digitalWrite(L_1, LOW); sendStatus1 = true; }
#endif
#ifdef CH_2
  bool sendStatus2 = false;
  int  SS2;
  unsigned long count2 = 0;
  Ticker btn_timer2;
  void channel2_on() { if (!isRain) digitalWrite(L_2, HIGH); else DEBUG_PRINTLN("It's raining now! Can't turn Relay 2 ON "); sendStatus2 = true; }
  void channel2_off() { digitalWrite(L_2, LOW); sendStatus2 = true; }
#endif
#ifdef CH_3
  bool sendStatus3 = false;
  int  SS3;
  unsigned long count3 = 0;
  Ticker btn_timer3;
  void channel3_on() { if (!isRain) digitalWrite(L_3, HIGH); else DEBUG_PRINTLN("It's raining now! Can't turn Relay 3 ON "); sendStatus3 = true; }
  void channel3_off() { digitalWrite(L_3, LOW); sendStatus3 = true; }
#endif
#ifdef CH_4
  bool sendStatus4 = false;
  int  SS4;
  unsigned long count4 = 0;
  Ticker btn_timer4;
  void channel4_on() { if (!isRain) digitalWrite(L_4, HIGH); else DEBUG_PRINTLN("It's raining now! Can't turn Relay 4 ON "); sendStatus4 = true; }
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
PubSubClient mqttClient(wifiClient);




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

  // load custom params
  EEPROM.begin(512);
  EEPROM.get(30, settings);
  EEPROM.end();

  WiFiManagerParameter custom_text("<p>MQTT username, password, broker IP address and broker port</p>");
  WiFiManagerParameter custom_mqtt_server("mqtt-server", "MQTT Broker IP", settings.mqttServer, STRUCT_CHAR_ARRAY_SIZE);
  WiFiManagerParameter custom_mqtt_user("mqtt-user", "MQTT User", settings.mqttUser, STRUCT_CHAR_ARRAY_SIZE);
  WiFiManagerParameter custom_mqtt_password("mqtt-password", "MQTT Password", settings.mqttPassword, STRUCT_CHAR_ARRAY_SIZE, "type = \"password\"");
  WiFiManagerParameter custom_mqtt_port("mqtt-port", "MQTT Broker Port", settings.mqttPort, 6);

  WiFiManagerParameter custom_text2("<p>SNTP servers name and timezone</p>");
  WiFiManagerParameter custom_sntp_server1("sntp-server1", "europe.pool.ntp.org", settings.sntpServer1, STRUCT_CHAR_ARRAY_SIZE);
  WiFiManagerParameter custom_sntp_server2("sntp-server2", "time.nist.gov", settings.sntpServer2, STRUCT_CHAR_ARRAY_SIZE);
  WiFiManagerParameter custom_time_zone("time-zone", "-1", settings.timeZone, 4);
  WiFiManager wifiManager;

  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_text2);
  wifiManager.addParameter(&custom_sntp_server1);
  wifiManager.addParameter(&custom_sntp_server2);
  wifiManager.addParameter(&custom_time_zone);

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(180);
  // set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  if (!wifiManager.autoConnect("Watering")) {
	  ///ESP.reset();
	  //DEBUG_PRINT(" .");
	  DEBUG_PRINT(F("Couldn't connect..."));
	  delay(30000);
  }

  if (shouldSaveConfig) {
	  strcpy(settings.mqttServer, custom_mqtt_server.getValue());
	  strcpy(settings.mqttUser, custom_mqtt_user.getValue());
	  strcpy(settings.mqttPassword, custom_mqtt_password.getValue());
	  strcpy(settings.mqttPort, custom_mqtt_port.getValue());
	  strcpy(settings.sntpServer1, custom_sntp_server1.getValue());
	  strcpy(settings.sntpServer2, custom_sntp_server2.getValue());
	  strcpy(settings.timeZone, custom_time_zone.getValue());

	  EEPROM.begin(512);
	  EEPROM.put(30, settings);
	  EEPROM.end();
  }


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
  mqttClient.set_server(settings.mqttServer, atoi(settings.mqttPort));
  /*WiFi.mode(WIFI_STA);
  WiFi.hostname(UID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);*/
  ArduinoOTA.setHostname(UID);
  ArduinoOTA.onStart([]() {
    OTAupdate = true;
    blinkLED(LED, 400, 2);
    digitalWrite(LED, HIGH);
    DEBUG_PRINTLN("OTA Update Initiated . . .");
  });
  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN("\nOTA Update Ended . . .s");
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
    if (error == OTA_AUTH_ERROR) DEBUG_PRINTLN(". . . . . . . . . . . . . . . Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DEBUG_PRINTLN(". . . . . . . . . . . . . . . Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DEBUG_PRINTLN(". . . . . . . . . . . . . . . Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINTLN(". . . . . . . . . . . . . . . Receive Failed");
    else if (error == OTA_END_ERROR) DEBUG_PRINTLN(". . . . . . . . . . . . . . . End Failed");
  });
  ArduinoOTA.begin();
  DEBUG_PRINTLN(HEADER);
  DEBUG_PRINT("\nUnit ID: ");
  DEBUG_PRINT(UID);
  DEBUG_PRINT("\nConnecting to "); DEBUG_PRINT(WIFI_SSID); DEBUG_PRINT(" Wifi"); 
  
  
 /* while ((WiFi.status() != WL_CONNECTED) && kRetries --) {
    delay(500);
    DEBUG_PRINT(" .");
  }*/
  if (WiFi.status() == WL_CONNECTED) {  
    DEBUG_PRINTLN(F(" DONE"));
    DEBUG_PRINT(F("IP Address is: ")); DEBUG_PRINTLN(WiFi.localIP());
	DEBUG_PRINT(F("Connecting to "));DEBUG_PRINT(MQTT_SERVER);DEBUG_PRINT(F(" Broker . ."));
    delay(500);
    while (!mqttClient.connect(MQTT::Connect(UID).set_keepalive(90).set_auth(MQTT_USER, MQTT_PASS)) && kRetries --) {
      DEBUG_PRINT(F(" ."));
      delay(1000);
    }
	
    if(mqttClient.connected()) {
      DEBUG_PRINTLN(" DONE");
      DEBUG_PRINTLN("\n---------------------  Logs  ---------------------");
      DEBUG_PRINTLN();
      mqttClient.subscribe(MQTT_TOPIC);
      blinkLED(LED, 40, 8);
      digitalWrite(LED, LOW);	 
	  //	  
	  DEBUG_PRINTLN("Setting time via NTP ...");
#ifdef NTP_SERVER1
	 // ntpServerName = NTP_SERVER1;
#endif // NTP_SERVER1

	  WiFi.hostByName(ntpServerName.c_str(), timeServerIP);
	  setTime(getNTPtime());
	  if (timeStatus() == timeSet) {
		  DEBUG_PRINT("Time is set. Current timestamp is ");
		  DEBUG_PRINTLN(now());
		  setSyncProvider(getNTPtime);
	  }
    }
    else {
      DEBUG_PRINTLN(" FAILED!");
      DEBUG_PRINTLN("\n--------------------------------------------------");
      DEBUG_PRINTLN();
    }
  }
  else {
    DEBUG_PRINTLN(" WiFi FAILED!");
    DEBUG_PRINTLN("\n--------------------------------------------------");
    DEBUG_PRINTLN();
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
	  DEBUG_PRINTLN("Time is not set! Can't initialize timers.");
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
        DEBUG_PRINTLN("\n\nSonoff Rebooting . . . . . . . . Please Wait"); 
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
      DEBUG_PRINTLN("mqtt broker connection . . . . . . . . . . OK");
    } 
    else {
      DEBUG_PRINTLN("mqtt broker connection . . . . . . . . . . LOST");
	  DEBUG_PRINT("Reconnecting ");
	  while (!mqttClient.connect(MQTT::Connect(UID).set_keepalive(90).set_auth(MQTT_USER, MQTT_PASS)) && kRetries--) {
		  Serial.print(" .");
		  delay(1000);
	  }
      requestRestart = true;
    }
  }
  else { 
    DEBUG_PRINTLN("WiFi connection . . . . . . . . . . LOST");
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
        DEBUG_PRINTLN("Relay 1 . . . . . . . . . . . . . . . . . . OFF");
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
      DEBUG_PRINTLN("Relay 1 . . . . . . . . . . . . . . . . . . ON");
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
        DEBUG_PRINTLN("Relay 2 . . . . . . . . . . . . . . . . . . OFF");
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
      DEBUG_PRINTLN("Relay 2 . . . . . . . . . . . . . . . . . . ON");
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
        DEBUG_PRINTLN("Relay 3 . . . . . . . . . . . . . . . . . . OFF");
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
      DEBUG_PRINTLN("Relay 3 . . . . . . . . . . . . . . . . . . ON");
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
        DEBUG_PRINTLN("Relay 4 . . . . . . . . . . . . . . . . . . OFF");
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
      DEBUG_PRINTLN("Relay 4 . . . . . . . . . . . . . . . . . . ON");
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
		DEBUG_PRINTLN("Time needs synchronization. Updating..."); 
		WiFi.hostByName(ntpServerName.c_str(), timeServerIP);
		getNTPtime();
	}
  }
}


