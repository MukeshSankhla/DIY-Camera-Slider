#include <logo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Define pins
#define stepPin 23
#define dirPin 5
#define limitSwitchPin 4

// Pushbutton pins
#define startStopButtonPin 17
#define homeButtonPin 16
#define speedButtonPin 6
#define microstepButtonPin 7

// Microstepping pins
#define MS1 21
#define MS2 22

// Display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool reverseDirection = false;
bool sliderRunning = false;
bool homing = false;
bool dirChange = true;
int stepDelay = 500;
int microstepMode = 1;                         // Start with 1/2 microstepping

unsigned long lastButtonPressTime = 0;
unsigned long buttonDebounceDelay = stepDelay / 1000.0 * 1000;

void setup() {
  Serial.begin(115200);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  // Set pin modes
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(limitSwitchPin, INPUT_PULLUP);
  pinMode(startStopButtonPin, INPUT_PULLUP);
  pinMode(homeButtonPin, INPUT_PULLUP);
  pinMode(speedButtonPin, INPUT_PULLUP);
  pinMode(microstepButtonPin, INPUT_PULLUP);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);

  // Initialize microstepping and display
  setMicrostepping(microstepMode);
  display.clearDisplay();
  display.drawBitmap(0, 0, logo_1, 128, 64, 1);
  display.display();
  delay(2000);
}

void loop() {
  // Handle start/stop button
  if (digitalRead(startStopButtonPin) == LOW) {
    sliderRunning = !sliderRunning;
    homing = false;
    dirChange = true;
    delay(200); // Small delay to avoid bouncing effect
    updateDisplay();
  }

  // Handle homing button
  if (digitalRead(homeButtonPin) == LOW) {
    homing = true;
    dirChange = true;
    sliderRunning = false;
    setMicrostepping(1); // 1/2 microstepping for homing
    stepDelay = 500;
    delay(200); // Small delay to avoid bouncing effect
    updateDisplay();
  }

  // Handle speed increment button
  if (!sliderRunning && digitalRead(speedButtonPin) == LOW) {
    stepDelay += 500;
    if (stepDelay > 10000) stepDelay = 500;
    delay(200); // Small delay to avoid bouncing effect
  }

  // Handle microstepping toggle button
  if (!sliderRunning && digitalRead(microstepButtonPin) == LOW) {
    microstepMode = (microstepMode % 4) + 1;
    setMicrostepping(microstepMode);
    delay(200); // Small delay to avoid bouncing effect
  }

  // If the slider is running, move the motor
  if (sliderRunning || homing) {
    digitalWrite(dirPin, reverseDirection ? HIGH : LOW);

    // Generate step pulses
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepDelay);
  }
  else{
    updateDisplay();
  }

  unsigned long currentTime = millis();
  
  // Check limit switch with debounce logic
  if(dirChange){
    if ((currentTime - lastButtonPressTime) >= buttonDebounceDelay) {
      if (digitalRead(limitSwitchPin) == LOW) {
        reverseDirection = !reverseDirection;   // Toggle the direction
        lastButtonPressTime = currentTime; // Update the limit switch debounce timer
        updateDisplay();
      }
    }
  }

  // Stop homing if limit switch is triggered
  if (homing && digitalRead(limitSwitchPin) == LOW) {
    homing = false;
    dirChange = false;
    sliderRunning = false;
    reverseDirection = !reverseDirection;
    updateDisplay();
  }
}

void setMicrostepping(int mode) {
  switch (mode) {
    case 1: // 1/2 step
      digitalWrite(MS1, HIGH);
      digitalWrite(MS2, LOW);
      break;
    case 2: // 1/4 step
      digitalWrite(MS1, LOW);
      digitalWrite(MS2, HIGH);
      break;
    case 3: // 1/8 step
      digitalWrite(MS1, LOW);
      digitalWrite(MS2, LOW);
      break;
    case 4: // 1/16 step
      digitalWrite(MS1, HIGH);
      digitalWrite(MS2, HIGH);
      break;
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Display the slider status
  display.setCursor(0, 0);
  display.print("Slider: ");
  display.print(sliderRunning ? "Running" : (homing ? "Homing" : "Stopped"));

  // Display the speed (step delay)
  display.setCursor(0, 16);
  display.print("Speed: ");
  display.print(stepDelay);
  display.print(" us");

  // Display the microstepping mode
  display.setCursor(0, 32);
  display.print("Microstep: ");
  switch (microstepMode) {
    case 1:
      display.print("1/2");
      break;
    case 2:
      display.print("1/4");
      break;
    case 3:
      display.print("1/8");
      break;
    case 4:
      display.print("1/16");
      break;
  }

  // Display the direction
  display.setCursor(0, 48);
  display.print("Dir: ");
  display.print(reverseDirection ? "Reverse" : "Forward");

  display.display();
}
