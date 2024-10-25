int interruptPin = 2;//set the interupt pin
volatile bool interruptFlag = false;

int call_count=1;//flags 
int first_interupt_flag=1;//intilizing gsm interuptflag
String phoneNumber="+919080991747";
#define TINY_GSM_MODEM_SIM800
//we cannot directly uses the hardware serial of arduino(serial data of arduino and gsm collide ) so i use the software serial 
#include <SoftwareSerial.h>
/*Sending and receiving SMS messages
Making and receiving voice calls
Connecting to the internet via GPRS
Using TCP/UDP protocols for communication
Handling MQTT communication*/
#include <TinyGsmClient.h>
//PubSubClient is a popular MQTT client library that allows you to connect your Arduino to an MQTT broker and publish or subscribe to MQTT topics.
#include <PubSubClient.h>

// Define software serial pins for GSM module
SoftwareSerial gsmSerial(7, 8);  // RX, TX

/*Debugging Output: When you define TINY_GSM_DEBUG and set it to Serial, the TinyGSM library will print debug information to the serial monitor. 
This can include details about the AT commands being sent to the GSM module, responses received, and other internal processes.

Serial Monitor Viewing: By setting it to Serial, you can view the debugging information on your computer's serial monitor 
(e.g., using the Arduino IDE's Serial Monitor) to understand what's happening in the background when your Arduino communicates with the GSM module.*/

#define TINY_GSM_DEBUG Serial

// Buzzer Pin
const int buzzerPin = 23;

// GSM and MQTT settings
const char* broker = "new-iot.cloud.shiftr.io";
const char* mqttUsername = "new-iot";
const char* mqttPassword = "dn3WXRPJD77svMgG";
const char* apn = "airtelgprs.com";
const char* gprsUser = "";
const char* gprsPass = "";

const char* topicOutput1 = "/buzz/sig";//to to give input
//const char* topicState = "/buzz/state";//to get the state

// GSM and MQTT clients
TinyGsm modem(gsmSerial);
TinyGsmClient gsmClient(modem);
PubSubClient mqttClient(gsmClient);



void setup() {
  pinMode(interruptPin, INPUT);// set as interupt pin in input
  Serial.begin(9600);  // Initialize serial communication

  // Attach interrupt to pin 2, trigger on RISING signal and FALLING signal
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, RISING);// interupt activate when the state chages

  gsmSerial.begin(9600);       // Start serial communication for GSM module
  pinMode(buzzerPin, OUTPUT);  // Set buzzer pin as output

  Serial.println("Initializing GSM...");
  if (!initializeGSM()) {
    Serial.println("GSM initialization failed.");
    while (true)
      ;
  }

  Serial.println("Connecting to GPRS...");
  if (!connectGPRS()) {
    Serial.println("GPRS connection failed.");
    while (true)
      ;
  }

  mqttClient.setServer(broker, 1883);
  mqttClient.setCallback(mqttCallback);

  Serial.println("Connecting to MQTT...");
  if (!connectMQTT()) {
    Serial.println("MQTT connection failed.");
    while (true)
      ;
  }

  Serial.println("Setup complete");
}

void loop() {
  // Handle MQTT connection
  if (!mqttClient.connected()) {
    reconnectMQTT();

  }
  mqttClient.loop();


    if (interruptFlag) {//only interupt flag is true
      Serial.println("Interrupt detected!");
      interruptFlag = false;  // Reset interrupt flag
      handleGSMEvent();       // Handle GSM event
   
    }
  

  delay(500);  // Add a small delay to prevent flooding
    if (gsmSerial.available() > 0) {// check for the data in gsmserial
      String incomingData = gsmSerial.readString();// read data and store it as string
      //call first time buzzer on and call second time buzzer off
      if (incomingData.indexOf("RING") != -1) {// string is RING then call in occured
        Serial.println("Incoming call detected.");
        if(call_count%2==1){// odd for on
        digitalWrite(buzzerPin, HIGH);// buzzer on
        Serial.println("buzzer on");
        delay(1000);
        gsmSerial.println("ATH\r");
        SendMessage("buzzer on",phoneNumber);//send on  message
       
        }
        else// even for off
        {
        digitalWrite(buzzerPin, LOW);// buzzer off
        Serial.println("buzzer off");
        delay(1000);
        gsmSerial.println("ATH\r");
        SendMessage("buzzer off",phoneNumber);// send off message
        
        }
         call_count++;// add the 1 to call_count
       
        
      }
       else if(incomingData.indexOf("ON") != -1)// if on through sms
      {
         digitalWrite(buzzerPin, HIGH);  // Turn the buzzer on
        Serial.println("Buzzer on sms");
        
        // Optional: Send an SMS indicating that the buzzer is on
        SendMessage("Buzzer on sms",phoneNumber);

        
      
      }
      else if(incomingData.indexOf("OFF") != -1)// if off through sms
      {
         digitalWrite(buzzerPin, LOW);  // Turn the buzzer on
        Serial.println("Buzzer off sms");

        // Optional: Send an SMS indicating that the buzzer is on
        SendMessage("Buzzer off sms",phoneNumber);
        delay(3000);// small dely
        reconnectMQTT();// connect to mqtt
        

      }
      
  
    }


  }



bool initializeGSM() {
  modem.restart();//used to reset the gsm andstarts with a clean state after an unexpected error or disconnection
  //this was the reson why the ri pin get high after intiliztion but we need this function for unexpected events
  delay(1000);
  //optional
  String modemInfo = modem.getModemInfo();// get the modem info
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);
  return true;
}

bool connectGPRS() {//for connected to mqtt we need gprs
  Serial.print("Connecting to APN: ");
  Serial.println(apn);
  for (int i = 0; i < 5; i++) {  // Retry 5 times
    if (modem.gprsConnect(apn, gprsUser, gprsPass)) {// connecting using the apn
      Serial.println("GPRS connected");
      return true;
    }
    Serial.println("GPRS connection failed, retrying...");// if not connected wait for 5 sec and retry
    delay(5000);
  }
  return false;// return false
}

bool connectMQTT() {// connected to the mqtt
  Serial.print("Connecting to MQTT broker: ");
  Serial.println(broker);
  for (int i = 0; i < 5; i++) {  // Retry 5 times
    if (mqttClient.connect("ArduinoClient", mqttUsername, mqttPassword)) {// connecting to the mqt using the username and password (ArduinoClient)is the name of the client
      Serial.println("MQTT connected");
      mqttClient.subscribe(topicOutput1);// subscribe to the topic to get msg publish in the topic
      //mqttClient.subscribe(topicState);
      return true;
    }
    //fails retry 5 times in the interval of 5 sec
    Serial.print("MQTT connection failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" retrying...");
    delay(5000);
  }
  return false;
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {// check the mqtt is not connected
    Serial.print("Attempting MQTT connection...");
    if (connectMQTT()) {// connected to mqtt
      Serial.println("connected");
    } else {// fails retry every  5 sec
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
// read data from the mqtt broker
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];//add the data to messge string
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (String(topic) == topicOutput1) {//check for the messge published topic and subscribed topic are same
    
    if (message == "ON") {// the is ON on the buzzer
      digitalWrite(buzzerPin, HIGH);
      Serial.println("Buzzer ON via MQTT");
      SendMessage("Buzzer on mqtt",phoneNumber);
  
    } else if (message == "OFF") {// if the mmsg is off off the buzzer
      digitalWrite(buzzerPin, LOW);
      Serial.println("Buzzer OFF via MQTT");
      SendMessage("Buzzer off mqtt",phoneNumber);
    }
  }
}

void handleInterrupt() {// interupt handling
  first_interupt_flag++;//add 1 
  if(first_interupt_flag>3)//the interupt gets activatd only after the first because the initilization of gsm trigger the gsm at first time
  interruptFlag = true;  // Set interrupt flag when the interrupt occurs
}
// for interupt we cannot able to do all operation during interupt spo i set flag
void handleGSMEvent() {
  //Disconnect MQTT to handle GSM event
  mqttClient.disconnect();
  delay(1000);

  Serial.println("Handling GSM event...");
  
}

void SendMessage(String message,String phoneNumber) {
  gsmSerial.println("AT+CMGF=1");    // Sets the GSM Module in Text Mode
  delay(1000);  // Delay of 1000 milliseconds or 1 second
   gsmSerial.println("AT+CMGS=\"" + phoneNumber + "\"\r"); // Use the provided phone number
  delay(1000);
  gsmSerial.println(message); // The SMS text you want to send
  delay(100);

  Serial.println("Finish");
  gsmSerial.println((char)26); // ASCII code of CTRL+Z
  delay(1000);

  Serial.println("-> SMS Sent");
}
