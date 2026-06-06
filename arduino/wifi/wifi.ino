#include <SoftwareSerial.h>
#include <WiFiS3.h>
#include <WebSocketsClient.h>
#include <EEPROM.h>
#include "led.h"
#include "config.h"

String ssid;
String password;
int status = WL_IDLE_STATUS;

WebSocketsClient socket;
IPAddress ip;
char auth[] = "/?auth=" AUTH;
SoftwareSerial mySerial(2, 3);
unsigned long last = 0;

struct Config {
  int index;
  IPAddress ip;
};

Config config;

void onEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
      case WStype_CONNECTED:
        last = 0;
        mySerial.println("connected");
        Serial.println("connected");
        break;
      case WStype_DISCONNECTED:
        if (millis() - last >= 5000) {
          if (last > 0) {
            findReachableHost();
            WiFi.disconnect();
            mySerial.println("connection error");
            Serial.println("connection error");
            break;
          }

          mySerial.println("disconnected");
          Serial.println("disconnected");
          last = millis();
        }
        break;
      case WStype_PING:
        socket.sendPing(NULL);
      case WStype_TEXT:
        mySerial.println((char *)payload);
        Serial.println((char *)payload);
        break;
  }
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  EEPROM.get(0, config);
  loadLed();

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
}

void loop() {
  handleSerial(mySerial);
  handleSerial(Serial);

  socket.loop();
  animation();
}

String getWifiNetworks() {
  int numSsid = WiFi.scanNetworks();
  String ssidList = "wifi ";

  if (numSsid > 0)
    for (int i = 0; i < numSsid; i++) {
      String ssid = WiFi.SSID(i);
      ssidList += ssid;

      if (i < numSsid - 1) {
        ssidList += ",";
      }
    }

  return ssidList;
}

bool findReachableHost() {
  if (config.index < 26 || config.index > 99)
    config.index = 26;

  for (int i = config.index; i < config.index + 3; i++) {
    String host = "ws.chessmaster" + String(i) + ".lol";
    Serial.print("Trying: ");
    Serial.println(host);

    if (WiFi.hostByName(host.c_str(), ip) == 1) {
      Serial.print("Resolved IP: ");
      Serial.println(ip);
      config.ip = ip;

      WiFiSSLClient client;
      config.index = i;
      EEPROM.put(0, config);

      return true;
    } else Serial.println("DNS failed");
  }

  return false;
}

void handleSerial(Stream &serial) {
  if (serial.available()) {
    String input = serial.readStringUntil('\n');
    input.trim();

    if (input == "wifi") {
      String wifi = getWifiNetworks();
      serial.println(wifi);
    }

    if (input.startsWith("wifi ")) {
      String credentials = input.substring(5);
      int spaceIndex = credentials.indexOf(':');

      if (spaceIndex != -1) {
        String ssid = credentials.substring(0, spaceIndex);
        String password = credentials.substring(spaceIndex + 1);

        int status = WiFi.begin(ssid.c_str(), password.c_str());
        if (status == WL_CONNECTED) {
          findReachableHost();
          socket.begin(config.ip, 1707, auth);
          socket.onEvent(onEvent);
        } else {
          WiFi.disconnect();
          serial.println("connection error");
        }
      }
    }

    if (input.startsWith("move ")) {
      String move = input.substring(5);
      socket.sendTXT(move);
    }

    if (input == "start") {
      socket.sendTXT("start");
    }

    if (input == "exit") {
      socket.sendTXT("exit");
    }

  }
}