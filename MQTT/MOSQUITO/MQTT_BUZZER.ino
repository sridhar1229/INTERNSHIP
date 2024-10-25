#ifdef ESP8266
  #include <ESP8266WiFi.h>
#else
  #include <WiFi.h>
#endif
#include <PubSubClient.h>

// Define pin for the buzzer
#define BUZZER_PIN 5

// Define Wi-Fi and MQTT settings
const char* wifi_ssid = "moto g54 5G_3480";
const char* wifi_password = "9500697232";
const char* mqtt_server = "test.mosquitto.org";

// Define MQTT topic for controlling the buzzer
#define buzzer_topic "control/buzzer"

// Create objects for Wi-Fi and MQTT
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);  // Changed to a more common baud rate
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(BUZZER_PIN, OUTPUT); // Set buzzer pin as output
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off at start
}

void setup_wifi() {
  delay(10);
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
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("ESP8266Client")) {  // Change "ESP8266Client" to something unique if you have multiple clients
      Serial.println("connected");
      client.subscribe(buzzer_topic);
      Serial.print("Subscribed to topic: ");
      Serial.println(buzzer_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  if (message == "ON") {
    digitalWrite(BUZZER_PIN, HIGH); // Turn the buzzer on
    Serial.println("Buzzer turned ON");
  } else if (message == "OFF") {
    digitalWrite(BUZZER_PIN, LOW); // Turn the buzzer off
    Serial.println("Buzzer turned OFF");
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
