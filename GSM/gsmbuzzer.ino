#define TINY_GSM_MODEM_SIM800

#define SerialMon Serial
#define SerialAT Serial

// #define TINY_GSM_DEBUG SerialMon // Comment this line to disable debug output

const char apn[] = "airtelgprs.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

const char* broker = "yellowroarer326.cloud.shiftr.io";
const char* mqttUsername = "yellowroarer326";
const char* mqttPassword = "jrSOKfZYGNgdnS7a";

const char* topicOutput1 = "/buzz/sig";
const char* topic = "/buzz/state";

// SIM card PIN (leave empty if not defined)
const char simPIN[] = "";

int p = -1;
bool buzzerStateChanged = false; // Flag to indicate if the buzzer state has changed

#include <Wire.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

#define BUZZER_PIN 6

long lastReconnectAttempt = 0;

void mqttCallback(char* topic, byte* message, unsigned int len) 
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < len; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == "/buzz/sig") {
    Serial.print("Changing buzzer state to ");
    if (messageTemp == "on") {
      Serial.println("ON");
      digitalWrite(BUZZER_PIN, HIGH);
      if (p != 1) {
        p = 1;
        buzzerStateChanged = true; // Set the flag when the buzzer state changes
      }
    } else if (messageTemp == "off") {
      Serial.println("OFF");
      digitalWrite(BUZZER_PIN, LOW);
      if (p != 0) {
        p = 0;
        buzzerStateChanged = true; // Set the flag when the buzzer state changes
      }
    }
  }
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
  mqtt.subscribe(topicOutput1);
  SerialMon.print("Subscribed to topic: ");
  SerialMon.println(topicOutput1);

  return mqtt.connected();
}

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
  mqtt.setKeepAlive(120); // Set keep-alive interval to 120 seconds
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

  if (buzzerStateChanged) { // Check if the buzzer state has changed
    const char* status = (p == 1) ? "BUZZER ON" : "BUZZER OFF";
    SerialMon.print("Publishing: ");
    SerialMon.println(status);
    delay(1000);
    bool publishStatus = mqtt.publish(topic, status, true);
    if (publishStatus) {
      SerialMon.println("Publish successful");
    } else {
      SerialMon.println("Publish failed");
    }
    buzzerStateChanged = false; // Reset the flag after publishing the message
  }
}
