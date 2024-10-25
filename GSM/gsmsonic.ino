#include <SoftwareSerial.h>

// Define pins for SIM800 communication
const int sim800RxPin = 9; // RX pin of SIM800 connected to Arduino pin 2
const int sim800TxPin = 10; // TX pin of SIM800 connected to Arduino pin 3

SoftwareSerial serialSIM800(sim800RxPin, sim800TxPin); // RX, TX

// Define pins for the ultrasonic sensor
const int trigPin = 7;
const int echoPin = 8;

// Recipient's phone number
const char* recipientNumber = "+918940196616";

// Function prototypes
void sendSMS(const char* number, const char* message);
void resetSIM800();

void setup() 
{
  // Initialize hardware serial communication at 9600 baud rate
  Serial.begin(9600);
  
  // Initialize software serial communication with SIM800 at 9600 baud rate
  serialSIM800.begin(9600);

  // Wait for SIM800 to initialize (adjust delay as needed)
  delay(1000);

  // Print a message to the Serial Monitor indicating that setup is complete
  Serial.println("Setup Complete");

  // Set SMS mode to text
  serialSIM800.println("AT+CMGF=1");
  delay(1000);

  // Configure the module to send SMS data directly to the serial port when received
  serialSIM800.println("AT+CNMI=2,2,0,0,0");
  delay(1000);

  // Disable echo (necessary for some SIM800 modules)
  serialSIM800.println("ATE0");
  delay(1000);
}

// Loop and send SMS function here

// Rest of your code

void loop()
{
  updateSerial();
  long duration;
  int distance;

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculating the distance
  distance = duration * 0.034 / 2;

  // Prints the distance on the Serial Monitor
  Serial.print("Distance: ");
  Serial.println(distance);

  // Send the distance via SMS
  char message[50];
  sprintf(message, "Distance: %d cm", distance);
  sendSMS(recipientNumber, message);

  // Wait for 2 seconds before the next measurement
  delay(2000);
}

void sendSMS(const char* number, const char* message)
{
  Serial.println("Sending SMS...");

  // Flush any existing data
  while (serialSIM800.available()) {
    serialSIM800.read();
  }

  // Send the AT command to set the recipient's number
  serialSIM800.print("AT+CMGS=\"");
  serialSIM800.print(number);
  serialSIM800.println("\"");
  delay(2000); // Increased delay

  // Check if '>' prompt is received, indicating SIM800 is ready to accept message
  if (serialSIM800.find(">")) {
    Serial.println("Sending message...");
    // Send the message
    serialSIM800.print(message);
    delay(1000); // Increased delay

    // End AT command with Ctrl+Z
    serialSIM800.write(26);
    delay(5000); // Increased delay

    // Wait for the module to process the message (adjust delay as needed)
    delay(5000);

    // Check for a response from the SIM800 module
    if (serialSIM800.available()) {
      String response = serialSIM800.readString();
      Serial.println("SMS Response: " + response);
      if (response.indexOf("ERROR") != -1) {
        Serial.println("Error sending SMS");
        // Attempt to reset SIM800 module
        resetSIM800();
      } else {
        Serial.println("SMS sent successfully");
      }
    } else {
      Serial.println("No response from SIM800");
    }
  } else {
    Serial.println("No '>' prompt");
    // You might attempt recovery or retry logic here
    // Example: reset the SIM800 module and retry sending SMS
    // resetSIM800();
  }
}

void resetSIM800() {
  Serial.println("Resetting SIM800 module...");
  // Toggle SIM800 power pin or use AT+CFUN=1
  // Example: digitalWrite(SIM800_POWER_PIN, LOW);
  //          delay(1000);
  //          digitalWrite(SIM800_POWER_PIN, HIGH);
  serialSIM800.println("AT+CFUN=1"); // Software reset
  delay(5000); // Allow module to reset (adjust delay as needed)
}

void updateSerial()
{
  delay(500);
  while (Serial.available()) 
  {
    serialSIM800.write(Serial.read()); // Forward what Serial received to Software Serial Port
  }
  
  while (serialSIM800.available()) 
  {
    String message = serialSIM800.readString(); // Read the incoming message
    Serial.print(message); // Forward the message to Serial Monitor

    if (message.indexOf("On") != -1) 
    {
      digitalWrite(buzz, HIGH); // Turn the buzzer on
      sendSMS("Buzzer is now ON");
    } 
    else if (message.indexOf("Off") != -1) 
    {
      digitalWrite(buzz, LOW); // Turn the buzzer off
      sendSMS("Buzzer is now OFF");
    }
  }
}
