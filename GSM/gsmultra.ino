#define TINY_GSM_MODEM_SIM800

#define SerialMon Serial
#define SerialAT Serial

#define TINY_GSM_DEBUG SerialMon

const char apn[] = "airtelgprs.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

const char* broker = "yellowroarer326.cloud.shiftr.io";
const char* mqttUsername = "yellowroarer326";
const char* mqttPassword = "jrSOKfZYGNgdnS7a";

const char* topic = "/ultra";
const char simPIN[] = "";

#include <Wire.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <NewPing.h>

#define TRIGGER_PIN  5
#define ECHO_PIN     4
#define BUZZER_PIN 6
#define MAX_DISTANCE 200  // Maximum distance we want to measure (in centimeters)

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

long lastReconnectAttempt = 0;

void setup() {
  SerialMon.begin(9600);
  delay(10);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure the buzzer is off initially
  SerialMon.println("Wait...");

  SerialAT.begin(9600);
  delay(6000);

  SerialMon.println("Initializing modem...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

  if (simPIN[0] != '\0' && modem.getSimStatus() != 3) {
    modem.simUnlock(simPIN);
  }

  SerialMon.print("Connecting to APN: ");
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
  } else {
    SerialMon.println(" OK");
  }

  if (modem.isGprsConnected()) {
    SerialMon.println("GPRS connected");
  }

  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
  mqtt.setKeepAlive(60); // Set keep-alive interval to 60 seconds
}

void mqttCallback(char* topic, byte* message, unsigned int len) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < len; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

boolean mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(broker);

  boolean status = mqtt.connect("GsmClientN", mqttUsername, mqttPassword);

  if (status == false) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" success");
  mqtt.subscribe(topic);
  SerialMon.print("Subscribed to topic: ");
  SerialMon.println(topic);

  return mqtt.connected();
}

void loop() {
  if (!mqtt.connected()) {
    SerialMon.println("=== MQTT NOT CONNECTED ===");
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
    delay(100);
    return;
  }

  mqtt.loop();

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

  mqtt.publish(topic, String(distance).c_str(), true);
}
