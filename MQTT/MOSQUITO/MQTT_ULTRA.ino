#ifdef ESP8266
  #include <ESP8266WiFi.h>     /* WiFi library for ESP8266 */
#else
  #include <WiFi.h>            /* WiFi library for ESP32 */
#endif
#include <Wire.h>
#include <PubSubClient.h>
#include <NewPing.h>            /* Ultrasonic sensor library */

// Define pins for ultrasonic sensor
#define TRIGGER_PIN  5
#define ECHO_PIN     4
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters)
// Define Wi-Fi and MQTT settings
#define wifi_ssid "moto g54 5G_3480"
#define wifi_password "9500697232"
#define mqtt_server "192.168.253.87"

// Define MQTT topics
#define distance_topic "sensor/ultrasonic/distance_cm"

// Create objects for Wi-Fi, MQTT, and Ultrasonic sensor
WiFiClient espClient;
PubSubClient client(espClient);
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
  delay(10);
  // Connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Wait a few seconds between measurements
  delay(2000);

  // Read distance from ultrasonic sensor
  unsigned int distance = sonar.ping_cm();

  // Check if the reading is valid
  if (distance == 0) {
    Serial.println("Failed to read from ultrasonic sensor or out of range!");
    return;
  }

  // Print and publish distance
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  client.publish(distance_topic, String(distance).c_str(), true);
}
