#include <SoftwareSerial.h>
#include <RTC.h>
#include <WebSocketsClient.h>
#include <EEPROM.h>
#include <ArduinoBLE.h>
#include "led.h"
#include "config.h"

BLEService customService("19B10000-E8F2-537E-4F6C-D104768a1214");
BLEStringCharacteristic textCharacteristic("19B10001-E8F2-537E-4F6C-D104768a1214", BLERead | BLEWrite | BLENotify, 20);

String ssid;
String password;
int status = WL_IDLE_STATUS;

WebSocketsClient socket;
IPAddress ip;
char auth[] = "/?auth=" AUTH;
SoftwareSerial mySerial(2, 3);
unsigned long last = 0;
bool ble = false;

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
          if (last > 0 && WiFi.status() == WL_CONNECTED) {
            resolveDNS();
            WiFi.disconnect();
            status = WL_IDLE_STATUS;
            mySerial.println("connection error");
            Serial.println("connection error");
            break;
          }

          if (last == 0) {
            mySerial.println("disconnected");
            Serial.println("disconnected");
            last = millis();
          }
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

void syncTimeNTP() {
  RTC.begin();

  unsigned long epochTime = 0;
  int retries = 0;

  while (epochTime == 0 && retries < 10) {
    epochTime = WiFi.getTime();
    if (epochTime == 0) {
      delay(500);
      retries++;
    }
  }

  if (epochTime > 0) {
    RTCTime timeToSet(epochTime);
    RTC.setTime(timeToSet);
    Serial.println("RTC synchronized via NTP!");
  } else {
    Serial.println("Error NTP (Timeout)");
  }
}

int getYear() {
  RTCTime currentTime;
  RTC.getTime(currentTime);
  return currentTime.getYear() % 100;
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  EEPROM.get(0, config);
  loadLed();

  if (!BLE.begin()) {
    Serial.println("Errore di avvio del modulo BLE!");
    while (true);
  }

  BLE.setLocalName("ChessMaster");
  BLE.setDeviceName("ChessMaster");
  BLE.setAdvertisedService(customService);
  customService.addCharacteristic(textCharacteristic);
  BLE.addService(customService);

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
  BLEDevice central = BLE.central();

  if (central) {
    Serial.println(central.address());
    ble = true;
    mySerial.println("ble connected");
    Serial.println("ble connected");

    while (central.connected()) {
      handleBleMessages();
      handleSerial(mySerial);
      handleSerial(Serial);
      socket.loop();
      animation();
    }

    ble = false;
    mySerial.println("ble disconnected");
    Serial.println("ble disconnected");
    BLE.stopAdvertise();
    Serial.println("BLE stopped...");
  }

  handleSerial(mySerial);
  handleSerial(Serial);

  socket.loop();
  animation();
}

String lastValidSsidList = "wifi ";
int emptyScanCount = 0;

String getWifiNetworks() {
  int numSsid = WiFi.scanNetworks();

  if (numSsid > 0) {
    emptyScanCount = 0;

    String ssidList = "wifi ";
    for (int i = 0; i < numSsid; i++) {
      ssidList += WiFi.SSID(i);
      if (i < numSsid - 1) ssidList += ",";
    }
    lastValidSsidList = ssidList;
    return ssidList;
  } else {
    emptyScanCount++;
    if (emptyScanCount > 2) {
      lastValidSsidList = "wifi ";
    }

    return lastValidSsidList;
  }
}

void resolveDNS() {
  if (config.index < 26 || config.index > 99)
    config.index = getYear();

  int index = config.index > 26 ? config.index - 1 : config.index;
  for (int i = index; i < index + (config.index > 26 ? 3 : 2); i++) {
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

      break;
    } else Serial.println("DNS failed");
  }
}

void handleBleMessages() {
  if (!textCharacteristic.written()) return;

  String message = textCharacteristic.value();
  message.trim();
  if (message.length() == 0) return;

  mySerial.println(message);
  Serial.println(message);
}

void handleSerial(Stream &serial) {
  if (serial.available()) {
    if (WiFi.status() == WL_IDLE_STATUS && status == WL_CONNECTED) {
      status = WL_IDLE_STATUS;
      WiFi.disconnect();
      serial.println("disconnected");
      serial.println("connection error");
    }


    String input = serial.readStringUntil('\n');
    input.trim();

    if (input == "ble") {
      ble = !ble;
      if (ble) {
        BLE.advertise();
        Serial.println("BLE started...");
      } else {
        BLE.stopAdvertise();
        Serial.println("BLE stopped...");
      }
    }

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

        status = WiFi.begin(ssid.c_str(), password.c_str());
        if (status == WL_CONNECTED) {
          syncTimeNTP();
          // resolveDNS();
          socket.begin(config.ip, 1707, auth);
          socket.onEvent(onEvent);
        } else {
          WiFi.disconnect();
          serial.println("connection error");
        }
      }
    }

    if (input == "ip") {
      serial.println("ip " + config.ip.toString());
    }

    if (input == "year") {
      serial.println("year " + String(config.index));
    }

    if (input.startsWith("ip ")) {
      String ip = input.substring(3);

      if (config.ip.fromString(ip)) {
        serial.println("valid ip");
        if (WiFi.status() == WL_CONNECTED) {
          socket.disconnect();
          serial.println("disconnected");
          socket.begin(config.ip, 1707, auth);
          socket.onEvent(onEvent);
        }
      } else {
        serial.println("invalid ip");
        resolveDNS();
      }
    }

    if (input.startsWith("move ")) {
      String move = input.substring(5);
      if (ble) textCharacteristic.writeValue(move);
      else socket.sendTXT(move);
    }

    if (input.startsWith("level ")) {
      if (ble) textCharacteristic.writeValue(input);
      else socket.sendTXT(input);
    }

    if (input == "start") {
      socket.sendTXT("start");
    }

    if (input == "exit") {
      if (ble) textCharacteristic.writeValue("exit");
      else socket.sendTXT("exit");
    }

  }
}
