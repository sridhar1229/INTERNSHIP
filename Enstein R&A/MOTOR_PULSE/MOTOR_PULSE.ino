#define STEPS_PER_DEGREE (300000.0 / 360.0)  // Equals exactly 833.3333333
// Pin definitions
const int stepPin = 9;       // Step signal pin
const int dirPin = 8;        // Direction signal pin

unsigned long int steps;     // Total steps to move
double Degeree = 0;           // Degrees to move (including fractions)
int speed = 500;             // Speed in Hz
bool direction = true;       // Direction: true = CW, false = CCW

void setup() {
  // Initialize pins
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);

  // Start serial communication
  Serial.begin(9600);
  Serial.println("Enter the degrees, speed (Hz), and direction (CW=1/CCW=0):");
}

void loop() {
  // Check if serial input is available
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // Read input line
    parseInput(input); // Parse input values
    
    // Move the motor based on input
    setDirection(direction); 
    moveDegeree(Degeree, speed);
    
    // Indicate completion
    Serial.println("Motion Complete.");
    Serial.println("Enter next degrees, speed, and direction:");
  }
}

// Function to set direction
void setDirection(bool clockwise) {
  digitalWrite(dirPin, clockwise ? HIGH : LOW); // HIGH for CW, LOW for CCW
}

// Function to move a specified number of degrees
void moveDegeree(double Degeree, int speed) {
  // Scale degrees to steps
  steps = round(Degeree *STEPS_PER_DEGREE);  // 8333 steps per 0.1 degree
  int stepDelay = 60000000 / (speed * 300000); // Step delay for given speed
  
  // Perform steps
  for (unsigned long int i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(stepDelay / 2); // High pulse
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepDelay / 2); // Low pulse
  }
  
  // Output results
  Serial.print("Steps reached: ");
  Serial.println(steps);
  Serial.print("Degrees reached: ");
  Serial.println((double)steps /STEPS_PER_DEGREE,3); // Convert steps back to degrees
}

// Function to parse input from serial monitor
void parseInput(String input) {
  input.trim(); // Remove any extra spaces or newlines

  // Split the input into tokens
  int firstComma = input.indexOf(',');
  int secondComma = input.indexOf(',', firstComma + 1);

  // Extract and convert the values
  Degeree = input.substring(0, firstComma).toDouble();
  speed = input.substring(firstComma + 1, secondComma).toInt();
  direction = input.substring(secondComma + 1).toInt();

  // Debugging output
  Serial.print("Parsed Degrees: ");
  Serial.println(Degeree,3);
  Serial.print("Parsed Speed: ");
  Serial.println(speed);
  Serial.print("Parsed Direction: ");
  Serial.println(direction ? "CW" : "CCW");
}
