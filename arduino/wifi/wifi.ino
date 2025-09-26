#include <SoftwareSerial.h>
#include <WiFiS3.h>
#include <WebSocketsClient.h>
#include "led.h"

String ssid;
String password;
int status = WL_IDLE_STATUS;

WebSocketsClient socket;
char ip[] = "IP";
char auth[] = "/?auth=AUTH";
SoftwareSerial mySerial(2, 3);

void onEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
      case WStype_CONNECTED:
          mySerial.println("connected");
          Serial.println("connected");
          break;
      case WStype_DISCONNECTED:
          mySerial.println("disconnected");
          Serial.println("disconnected");
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

void handleSerial(Stream &serial) {
  if (serial.available()) {
    String input = serial.readStringUntil('\n');
    input.trim();

    if (input == "wifi") {
      serial.println(getWifiNetworks());
    }

    if (input.startsWith("wifi ")) {
      String credentials = input.substring(5);
      int spaceIndex = credentials.indexOf(':');

      if (spaceIndex != -1) {
        String ssid = credentials.substring(0, spaceIndex);
        String password = credentials.substring(spaceIndex + 1);

        int status = WiFi.begin(ssid.c_str(), password.c_str());
        if (status == WL_CONNECTED) {
          socket.begin(ip, 1707, auth);
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