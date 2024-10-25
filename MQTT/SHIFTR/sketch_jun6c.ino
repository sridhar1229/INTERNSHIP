#include <WiFi.h>
#include <MQTT.h>

const char ssid[] = "moto g54 5G_3480";
const char pass[] = "9500697232";
const char mqttUsername[] = "new-iot"; // Change to your MQTT username
const char mqttPassword[] = "dn3WXRPJD77svMgG"; // Change to your MQTT password

WiFiClient net;
MQTTClient client;

const int buzzerPin = 26; // Change to your actual buzzer pin number
const int trigPin = 2;   // Ultrasonic sensor trigger pin
const int echoPin = 4;   // Ultrasonic sensor echo pin

unsigned long lastMillis = 0;

void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" Connected to WiFi!");
}

void connectMQTT() {
  Serial.print("Connecting to MQTT...");
  while (!client.connect("arduino", mqttUsername, mqttPassword)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" Connected to MQTT!");

  client.subscribe("/buzzer/control");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("Incoming: " + topic + " - " + payload);
  if (topic == "/buzzer/control") {
    if (payload == "1") {
      digitalWrite(buzzerPin, HIGH); // Turn on buzzer
      client.publish("/buzzer/status", "ON"); // Publish buzzer status as 1 (ON)
    } else if (payload == "0") {
      digitalWrite(buzzerPin, LOW); // Turn off buzzer
      client.publish("/buzzer/status", "OFF"); // Publish buzzer status as 0 (OFF)
    }
  }
}

float measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  unsigned long duration = pulseIn(echoPin, HIGH);
  float distance = (duration * 0.0343) / 2; // Speed of sound is 343m/s, divide by 2 for one-way distance
  return distance;
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

  pinMode(buzzerPin, OUTPUT); // Set buzzer pin as output
  pinMode(trigPin, OUTPUT);   // Set ultrasonic sensor trigger pin as output
  pinMode(echoPin, INPUT);    // Set ultrasonic sensor echo pin as input

  connectWiFi();

  client.begin("new-iot.cloud.shiftr.io", net);
  client.onMessage(messageReceived);

  connectMQTT();
}

void loop() {
  client.loop();

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!client.connected()) {
    connectMQTT();
  }

  // Measure distance and publish
  float distance = measureDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  client.publish("/ultrasonic", String(distance));

  // Wait for a moment before taking next measurement
  delay(1000);
}
