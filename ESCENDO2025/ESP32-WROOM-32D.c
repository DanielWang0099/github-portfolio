#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h> // Include the ESP32Servo library

#define CONFIG_BUTTON_PIN 25
#define RUNNING_BUTTON_PIN 26
#define CONFIG_LED_PIN 27
#define RUNNING_LED_PIN 14
#define MOTOR_PIN 12 // Servo motor connected here
#define YELLOW_LED1_PIN 13
#define YELLOW_LED2_PIN 15
#define BUZZER_PIN 23

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {2, 0, 4, 16};
byte colPins[COLS] = {17, 5, 18, 19};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo setup
Servo servo; // Create a servo object

// Global variables
int intervalMedicine1 = 3600; // Default 1-hour interval (in seconds)
int intervalMedicine2 = 7200; // Default 2-hour interval (in seconds)
int remainingTime1 = intervalMedicine1; // Remaining time for Medicine 1
int remainingTime2 = intervalMedicine2; // Remaining time for Medicine 2
unsigned long lastUpdateMillis = 0; // For updating the countdown
unsigned long lastActivityMillis = 0; // For tracking user or system activity
bool lowConsumptionMode = false; // Flag for Low Consumption Mode
bool configurationMode = false;
bool runningModeActive = false; // Track if Running Mode is active

void setup() {
    pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP); // Use internal pull-up resistor
    pinMode(RUNNING_BUTTON_PIN, INPUT_PULLUP); // Use internal pull-up resistor
    pinMode(CONFIG_LED_PIN, OUTPUT);
    pinMode(RUNNING_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED1_PIN, OUTPUT);
    pinMode(YELLOW_LED2_PIN, OUTPUT);
    
    digitalWrite(CONFIG_LED_PIN, LOW);
    digitalWrite(RUNNING_LED_PIN, LOW);
    digitalWrite(YELLOW_LED1_PIN, LOW);
    digitalWrite(YELLOW_LED2_PIN, LOW);

    Serial.begin(115200);
    lcd.init();
    lcd.backlight();

    // Attach the servo to the motor pin
    servo.attach(MOTOR_PIN, 500, 2500); // Specify min/max pulse width (microseconds)
    servo.write(90); // Initialize servo at 90 degrees (neutral position)

    // Start with configuration mode
    configurationMode = true;
    enterConfigurationMode();
}

void loop() {
    // Check configuration button
    if (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
        wakeUp();
        configurationMode = true;
        enterConfigurationMode();
        return;
    }

    // Check running button
    if (digitalRead(RUNNING_BUTTON_PIN) == LOW) {
        wakeUp();
        if (!runningModeActive) { // Enter Running Mode only if not already active
            configurationMode = false;
            enterRunningMode();
        } else {
            Serial.println("Already in Running Mode. Continuing operation...");
        }
        return;
    }

    // Running Mode Logic
    if (!configurationMode) {
        if (millis() - lastUpdateMillis >= 1000) {
            updateCountdown();
            lastUpdateMillis = millis();
        }

        // Check for inactivity to enter Low Consumption Mode
        if (!lowConsumptionMode && (millis() - lastActivityMillis > 30000)) {
            enterLowConsumptionMode();
        }
    }
}

void enterConfigurationMode() {
    runningModeActive = false; // Deactivate Running Mode
    digitalWrite(CONFIG_LED_PIN, HIGH);
    digitalWrite(RUNNING_LED_PIN, LOW);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Config Mode:");
    lcd.setCursor(0, 1);
    lcd.print("Set Intervals");
    delay(2000);

    // Interval for Medicine 1
    lcd.clear();
    lcd.print("Medicine 1");
    intervalMedicine1 = getTimeFromKeypad("Med 1"); // Update intervalMedicine1

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Med 1 Interval:");
    lcd.setCursor(0, 1);
    lcd.print(formatTime(intervalMedicine1)); // Show correct time
    delay(2000);

    // Interval for Medicine 2
    lcd.clear();
    lcd.print("Medicine 2");
    intervalMedicine2 = getTimeFromKeypad("Med 2"); // Update intervalMedicine2

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Med 2 Interval:");
    lcd.setCursor(0, 1);
    lcd.print(formatTime(intervalMedicine2)); // Show correct time
    delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Config Complete");
    lcd.setCursor(0, 1);
    lcd.print("Press Run");
    delay(2000);

    digitalWrite(CONFIG_LED_PIN, LOW);
}

void enterRunningMode() {
    wakeUp();

    if (!runningModeActive) { // Initialize timers only on first entry to Running Mode
        remainingTime1 = intervalMedicine1; // Use configured interval for Medicine 1
        remainingTime2 = intervalMedicine2; // Use configured interval for Medicine 2
        runningModeActive = true; // Set Running Mode as active
    }

    digitalWrite(RUNNING_LED_PIN, HIGH);
    digitalWrite(CONFIG_LED_PIN, LOW);

    lcd.clear();
    lcd.print("Running Mode...");
    delay(2000);
}
void updateCountdown() {
    lcd.setCursor(0, 0);
    lcd.print("Med 1: ");
    lcd.print(formatTime(remainingTime1));
    lcd.print("   ");

    lcd.setCursor(0, 1);
    lcd.print("Med 2: ");
    lcd.print(formatTime(remainingTime2));
    lcd.print("   ");

    // Medicine 1 Countdown
    if (remainingTime1 > 0) {
        remainingTime1--;
    } else {
        wakeUp();
        triggerReminder(1);
        remainingTime1 = intervalMedicine1; // Reset timer
    }

    // Medicine 2 Countdown
    if (remainingTime2 > 0) {
        remainingTime2--;
    } else {
        wakeUp();
        triggerReminder(2);
        remainingTime2 = intervalMedicine2; // Reset timer
    }
}

void triggerReminder(int medicineNumber) {
    servo.attach(MOTOR_PIN, 500, 2500); // Re-attach the servo
    if (medicineNumber == 1) {
        digitalWrite(YELLOW_LED1_PIN, HIGH);
        lcd.clear();
        servo.write(0); // Move servo to 0 degrees
        lcd.setCursor(0, 0);
        lcd.print("Reminder: Med 1");
        delay(500);

    } else if (medicineNumber == 2) {
        digitalWrite(YELLOW_LED2_PIN, HIGH);
        lcd.clear();
        servo.write(180); // Move servo to 0 degrees
        lcd.setCursor(0, 0);
        lcd.print("Reminder: Med 2");
        delay(500);
    }

    lcd.setCursor(0, 1);
    lcd.print("Take your meds!");

    tone(BUZZER_PIN, 1000); // Start the buzzer
    delay(5000); // Keep the reminder active for 5 seconds
    noTone(BUZZER_PIN);

    servo.detach(); // Detach the servo after use
    digitalWrite(YELLOW_LED1_PIN, LOW);
    digitalWrite(YELLOW_LED2_PIN, LOW);
}

void enterLowConsumptionMode() {
    lowConsumptionMode = true;
    lcd.noBacklight(); // Turn off LCD backlight
    digitalWrite(CONFIG_LED_PIN, LOW); // Turn off LEDs
    digitalWrite(RUNNING_LED_PIN, LOW);
    Serial.println("Entering Low Consumption Mode...");
}

void wakeUp() {
    if (lowConsumptionMode) {
        lowConsumptionMode = false;
        lcd.backlight(); // Turn on LCD backlight
        digitalWrite(RUNNING_LED_PIN, HIGH); // Restore LEDs
        Serial.println("Exiting Low Consumption Mode...");
    }
    lastActivityMillis = millis(); // Reset inactivity timer
}

int getTimeFromKeypad(String medicineName) {
    int hours, minutes, seconds;

    lcd.clear();
    lcd.print(medicineName + ": Hours");
    lcd.setCursor(0, 1);
    hours = get2DigitInput();
    delay(500);

    lcd.clear();
    lcd.print(medicineName + ": Minutes");
    lcd.setCursor(0, 1);
    minutes = get2DigitInput();
    delay(500);

    lcd.clear();
    lcd.print(medicineName + ": Seconds");
    lcd.setCursor(0, 1);
    seconds = get2DigitInput();
    delay(500);

    return (hours * 3600) + (minutes * 60) + seconds;
}

int get2DigitInput() {
    String input = "";
    char key;
    while (input.length() < 2) { // Allow only 2 digits
        key = keypad.getKey();
        if (key >= '0' && key <= '9') {
            input += key;
            lcd.print(key); // Display the key pressed
        }
    }
    return input.toInt(); // Convert the 2-digit input to an integer
}

String formatTime(int totalSeconds) {
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    char buffer[9];
    sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
    return String(buffer);
}
