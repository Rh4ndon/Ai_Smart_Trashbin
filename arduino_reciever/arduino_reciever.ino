//pin usage as follow for tft:
//             CS  DC/RS  RESET  SDI/MOSI  SCK   LED    VCC     GND    
//Arduino Uno  A5   A3     A4       11      13   A0   5V/3.3V   GND

//Remember to set the pins to suit your display module!
#include <Servo.h>

// Include required libraries for TFT display
#include <LCDWIKI_GUI.h> //Core graphics library
#include <LCDWIKI_SPI.h> //Hardware-specific library

// Setup Software Serial for ESP32 communication
#include <SoftwareSerial.h>
SoftwareSerial mySerial(8, 9); // RX, TX
                              //Arduino Pin 8 (RX) → ESP32 TX (GPIO 13)
                              //Arduino Pin 9 (TX) → ESP32 RX (GPIO 12) 

// Hardware Pin Configurations
#define SERVO_PIN  7    // Pin for controlling the sorting servo motor
#define RELAY_PIN  6    // Pin for controlling the charging relay

// TFT Display Configuration
#define MODEL ILI9488_18  // Model of the TFT display
#define CS   A5    // Chip Select pin
#define CD   A3    // Command/Data pin
#define RST  A4    // Reset pin
#define LED  A0    // Backlight control pin (set to -1 if connected to 3.3V)

// Initialize TFT display with specified pins
LCDWIKI_SPI mylcd(MODEL, CS, CD, RST, LED); //model,cs,dc,reset,led

// Color definitions for TFT display
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// Initialize servo motor object
Servo trashServo;

// Timing variables for charging functionality
unsigned long timerStart = 0;      // Timestamp when charging starts
unsigned long timerDuration = 900000; // Duration for charging (15 minutes in milliseconds)
bool relayActive = false;          // Flag to track if charging is active
unsigned long lastUpdateTime = 0;   // Timestamp for display updates

void setup() {
  delay(5000); // Initial delay to ensure ESP32 has completed booting
  
  // Initialize serial communications
  Serial.begin(9600);      // Debug serial
  mySerial.begin(9600);    // ESP32 communication

  // Configure TFT display
  mylcd.Init_LCD();
  mylcd.Set_Rotation(1);
  mylcd.Fill_Screen(BLACK);

  // Configure servo motor
  trashServo.attach(SERVO_PIN);
  trashServo.write(90);    // Set initial position

  // Configure relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Ensure relay starts in OFF state

  // Display welcome message
  displayInitialText();
}

/**
 * Displays the initial welcome message and instructions on the TFT screen
 * Uses predefined text size and colors
 */
void displayInitialText() {
  mylcd.Fill_Screen(BLACK);
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Back_colour(BLACK);
  mylcd.Set_Text_Size(5);
  mylcd.Print_String("****************", 0, 10);
  mylcd.Print_String("Please Insert", 0, 65);
  mylcd.Print_String("PlasticBottleAnd", 0, 130);
  mylcd.Print_String("Press The Button", 0, 200);
  mylcd.Print_String("****************", 0, 270);
}

void loop() {
  // Check for incoming data from ESP32
  if (mySerial.available()) {
    // Read and clean up incoming data
    String result = mySerial.readString();
    result.trim();  // Remove whitespace
    result.replace("\r", ""); // Remove carriage return
    result.replace("\n", ""); // Remove newline

    // Debug output
    Serial.println("Received: '" + result + "'");
    Serial.print("Length of received: ");
    Serial.println(result.length());

    // Clear screen for new message
    mylcd.Fill_Screen(BLACK);
    
    // Handle plastic bottle detection
     
    if (result == "plastic_bottle") {
      Serial.println("Condition matched: plastic_bottle");
      mySerial.println("charging"); // Notify ESP32 charging is starting
      
      // Display acceptance message
      mylcd.Set_Text_colour(GREEN);
      mylcd.Set_Text_Size(5);
      mylcd.Print_String(" TRASH ACCEPTED", 0, 120);

      // Actuate servo to accept bottle
      trashServo.writeMicroseconds(3000);  // Open position
      delay(1000);
      trashServo.writeMicroseconds(1500);  // Close position

      // Start charging cycle
      mylcd.Fill_Screen(BLACK);
      mylcd.Set_Text_Size(4);
      mylcd.Print_String("    Charger on", 0, 100);
      mylcd.Print_String("    for 15 mins", 0, 150);
      delay(2000);
      
      // Initialize charging timer
      timerStart = millis();
      digitalWrite(RELAY_PIN, HIGH);  // Activate charging relay
      relayActive = true;

    } 
    else {
      // Handle non-plastic bottle detection
      Serial.println("Condition matched: not_plastic_bottle");
      
      // Display rejection message
      mylcd.Set_Text_colour(RED);
      mylcd.Set_Text_Size(5);
      mylcd.Print_String(" SORRY!", 0, 40);
      mylcd.Print_String(" TRASH REJECTED!", 0, 120);

      // Actuate servo to reject item
      trashServo.writeMicroseconds(500);  // Reject position
      delay(1000);
      trashServo.writeMicroseconds(1500);  // Return to center
      delay(3000);
      
      // Return to initial state
      mylcd.Fill_Screen(BLACK);
      mylcd.Set_Text_colour(WHITE);
      mylcd.Set_Text_Back_colour(BLACK);
      mylcd.Set_Text_Size(5);
      mylcd.Print_String("****************", 0, 10);
      mylcd.Print_String("Please Insert", 0, 65);
      mylcd.Print_String("PlasticBottleAnd", 0, 130);
      mylcd.Print_String("Press The Button", 0, 200);
      mylcd.Print_String("****************", 0, 270);
    }
  }

  // Handle charging timer and display updates
  if (relayActive) {
    unsigned long currentMillis = millis();
    unsigned long elapsedTime = currentMillis - timerStart;
    unsigned long remainingTime = (elapsedTime < timerDuration) ? timerDuration - elapsedTime : 0;

    // Check if charging time is complete
    if (remainingTime == 0) {
      // End charging cycle
      digitalWrite(RELAY_PIN, LOW);
      relayActive = false;
      
      // Display completion message
      mylcd.Fill_Screen(BLACK);
      mylcd.Set_Text_colour(WHITE);
      mylcd.Set_Text_Size(4);
      mylcd.Print_String("    Charger OFF", 0, 120);
      delay(2000);
      displayInitialText();
      mySerial.println("done_charging"); // Notify ESP32
      
    } else {
      // Update charging status display (once per second)
      if (millis() - lastUpdateTime >= 1000) {
        mySerial.println("charging"); // Keep ESP32 updated
        lastUpdateTime = millis();
        
        // Calculate remaining time
        int minutes = remainingTime / 60000;
        int seconds = (remainingTime % 60000) / 1000;

        // Update display with remaining time
        mylcd.Fill_Screen(BLACK);
        mylcd.Set_Text_Size(4);
        mylcd.Print_String("    Charger on", 0, 60);
        mylcd.Print_String("    Time Left:", 0, 120);
        mylcd.Set_Text_colour(CYAN);
        mylcd.Print_String("    " + String(minutes) + ":" + String(seconds), 0, 180);
      }
    }
  }
}