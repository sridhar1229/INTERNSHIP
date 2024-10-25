#ifdef ESP8266
  #include <ESP8266WiFi.h>     /* WiFi library for ESP8266 */
#else
  #include <WiFi.h>            /* WiFi library for ESP32 */
#endif
#include <MQTT.h>
#include <Arduino.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"

// WiFi credentials
#define WIFI_SSID         "moto g54 5G_3480"    
#define WIFI_PASS         "9500697232"

// Sinric Pro credentials
#define APP_KEY           "0dbfaadc-2e5b-447b-b59d-4f453e288beb"
#define APP_SECRET        "50a44a49-a614-47f3-8591-949a4059ccfd-5338ffda-063b-4b5f-a2a3-0540650ce907"
#define DEVICE_ID         "66644662888aa7f7a2354536"

// MQTT credentials
const char mqttUsername[] = "yellowroarer326";
const char mqttPassword[] = "jrSOKfZYGNgdnS7a";

// Pin definitions
#define LED_PIN 2   // D2 (built-in LED for most ESP32 boards)

// MQTT client
WiFiClient net;
MQTTClient client(256); // Buffer size can be adjusted if needed

// MQTT topics
const char* ledControlTopics = "/led/control";
const char* ledStatusTopics = "/led/status";

// State tracking to avoid unnecessary updates
bool ledState = false;
bool updateFromMQTT = false;
bool updateFromSinricPro = false;

void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.printf(" connected!\n[WiFi]: IP-Address is %s\n", WiFi.localIP().toString().c_str());
}

void connectMQTT() {
  Serial.print("Connecting to MQTT...");
  while (!client.connect("ESP32Client", mqttUsername, mqttPassword)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" Connected to MQTT!");
  client.subscribe(ledControlTopics);
}

void messageReceived(String &topic, String &payload) {
  Serial.println("Incoming: " + topic + " - " + payload);
  
  if (topic == ledControlTopics) {
    if (payload == "b1 - on") {
      ledState = true;
      updateFromMQTT = true;
      digitalWrite(LED_PIN, HIGH); // Turn on LED
      client.publish(ledStatusTopics, "B1 ON");
      SinricProSwitch& mySwitch = SinricPro[DEVICE_ID];
      mySwitch.sendPowerStateEvent(ledState);
    } else if (payload == "b1 - off") {
      ledState = false;
      updateFromMQTT = true;
      digitalWrite(LED_PIN, LOW); // Turn off LED
      client.publish(ledStatusTopics, "B1 OFF");
      SinricProSwitch& mySwitch = SinricPro[DEVICE_ID];
      mySwitch.sendPowerStateEvent(ledState);
    }
    updateFromMQTT = false;
  }
}

bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("Device %s turned %s\n", deviceId.c_str(), state ? "on" : "off");
  ledState = state;
  updateFromSinricPro = true;
  digitalWrite(LED_PIN, state ? HIGH : LOW); // Turn LED on or off
  client.publish(ledStatusTopics, state ? "B1 ON" : "B1 OFF");
  updateFromSinricPro = false;
  return true;
}

void setup() {
  Serial.begin(9600); // Use a higher baud rate for better debugging output
  
  // Connect to WiFi
  connectWiFi();

  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT); // Set LED pin as output
  digitalWrite(LED_PIN, LOW); // Initialize LED to be off

  // Setup SinricPro
  Serial.println("Setting up SinricPro...");
  SinricProSwitch& mySwitch = SinricPro[DEVICE_ID];
  mySwitch.onPowerState(onPowerState);
  SinricPro.onConnected([]() {
    Serial.println("Connected to SinricPro!");
  });
  SinricPro.onDisconnected([]() {
    Serial.println("Disconnected from SinricPro!");
  });
  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);

  // Setup MQTT client
  Serial.println("Setting up MQTT...");
  client.begin("yellowroarer326.cloud.shiftr.io", 1883, net);
  client.onMessage(messageReceived);

  // Connect to MQTT
  connectMQTT();
}

void loop() {
  // Handle MQTT client
  client.loop();

  // Handle SinricPro
  SinricPro.handle();

  // Reconnect to WiFi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // Reconnect to MQTT if disconnected
  if (!client.connected()) {
    connectMQTT();
  }

  // Non-blocking delay
  delay(100); // Small delay to yield to other processes
}
