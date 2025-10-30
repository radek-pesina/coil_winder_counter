#include <LiquidCrystal.h>

/***************************************************************************************************
| LCD Pin | Name     | Connect To         | Notes                                                  |
| ------- | -------- | ------------------ | ------------------------------------------------------ |
| 1       | VSS      | GND                | Power GND                                              |
| 2       | VDD      | 5V                 | Power VCC                                              |
| 3       | VO       | Middle of 10k pot  | Contrast. Other legs to 5V & GND (or fixed ~2k to GND) |
| 4       | RS       | Arduino **D7**     | Register Select                                        |
| 5       | RW       | GND                | Always write (no reading needed)                       |
| 6       | E        | Arduino **D6**     | Enable                                                 |
| 7       | D0       | —                  | Not used in 4-bit mode                                 |
| 8       | D1       | —                  | Not used in 4-bit mode                                 |
| 9       | D2       | —                  | Not used in 4-bit mode                                 |
| 10      | D3       | —                  | Not used in 4-bit mode                                 |
| 11      | D4       | Arduino **D5**     | Data bit 4                                             |
| 12      | D5       | Arduino **D4**     | Data bit 5                                             |
| 13      | D6       | Arduino **D3**     | Data bit 6                                             |
| 14      | D7       | Arduino **D8**     | Data bit 7                                             |
| 15      | A (LED+) | 5V (with resistor) | Backlight power                                        |
| 16      | K (LED−) | GND                | Backlight ground                                       |
***************************************************************************************************/
// LCD pin mapping: RS, EN, D4, D5, D6, D7
LiquidCrystal lcd(7, 6, 5, 4, 3, 8);  // RS=7, EN=6, D4=5, D5=4, D6=3, D7=8

/******************************************************************************
| Component              | Pin on Arduino Uno | Notes                         |
| --------------         | ------------------ | ----------------------------- |
| Increment Button       | D2                 | With internal pull-up enabled |
|                        | Other leg          | GND                           |
| Serial logging         | USB                | Serial Monitor                |
|                        | GND                | GND                           |
| buzzer            +ve  | D13                |                               |
|                   -ve  | Arduino GND        | GND                           |
| 49E Hall Sensor    DO  | D12                | Hall sensor digital out       |
|                   VCC  | Arduino VCC        | 5V                            |
|                   GND  | Arduino GND        | GND                           |
******************************************************************************/

const int sensorPin = 12;     // 49E Hall sensor digital output
const int buttonPin = 2;     // button pin
const int buzzerPin = 13;    // buzzer pin
unsigned long targetCount = 0;  // target number of coils
unsigned long currentCount = 0;  // current coil count
bool settingTarget = true;    // initial state for setting target
bool buzzerActive = false;    // buzzer state

int lastSensorState = HIGH;   // for edge detection
int lastButtonState = HIGH;   // for button edge detection
unsigned long buttonPressStartTime = 0;  // for long press detection
const unsigned long LONG_PRESS_TIME = 2000;  // 2 seconds for acceleration
const unsigned long FAST_INCREMENT_INTERVAL = 200;  // 5 times per second (1000/5=200ms)
unsigned long lastFastIncrementTime = 0;

// Setup printf() support on Arduino Uno
int serial_putchar(char c, FILE *)
{
  Serial.write(c);
  return c;
}
FILE serial_stdout;

void setup()
{
  pinMode(sensorPin, INPUT_PULLUP);   // use internal pull-up
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  lcd.begin(16, 2);
  lcd.print("Set Target:");
  lcd.setCursor(0, 1);
  lcd.print("Count: 0");

  Serial.begin(9600);

  // Enable printf()
  fdev_setup_stream(&serial_stdout, serial_putchar, NULL, _FDEV_SETUP_WRITE);
  stdout = &serial_stdout;
}

void loop()
{
  // Handle button input for target setting or buzzer reset
  int buttonState = digitalRead(buttonPin);

  if (settingTarget) {
    // Target setting mode
    if (buttonState == LOW && lastButtonState == HIGH) {
      // Button just pressed
      buttonPressStartTime = millis();
      targetCount++;
      updateDisplay();
      delay(50); // Initial debounce
    }
    else if (buttonState == LOW && (millis() - buttonPressStartTime) > LONG_PRESS_TIME) {
      // Long press - fast increment
      if (millis() - lastFastIncrementTime >= FAST_INCREMENT_INTERVAL) {
        targetCount += 5;
        updateDisplay();
        lastFastIncrementTime = millis();
      }
    }
    else if (buttonState == HIGH && lastButtonState == LOW) {
      // Button released
      if (millis() - buttonPressStartTime > LONG_PRESS_TIME) {
        delay(500); // Additional delay after fast increment
      }
    }

    // Exit target setting mode after 3 seconds of no button activity
    if (buttonState == HIGH && lastButtonState == HIGH &&
        targetCount > 0 && millis() - buttonPressStartTime > 3000) {
      settingTarget = false;
      currentCount = targetCount;
      lcd.clear();
      lcd.print("Winding:");
      updateDisplay();
    }
  }
  else {
    // Normal counting mode
    // Handle sensor reading
    int sensorState = digitalRead(sensorPin);
    if (sensorState == LOW && lastSensorState == HIGH) {
      if (currentCount > 0) {
        currentCount--;
        updateDisplay();
        printf("Coils remaining: %lu\n", currentCount);

        // Activate buzzer when count reaches zero
        if (currentCount == 0) {
          buzzerActive = true;
        }
      }
      delay(50); // debounce / slow polling
    }
    lastSensorState = sensorState;

    // Handle buzzer
    if (buzzerActive) {
      // Beep pattern: 500ms on, 500ms off
      digitalWrite(buzzerPin, (millis() / 500) % 2 == 0 ? HIGH : LOW);

      // Check for button press to stop buzzer
      if (buttonState == LOW && lastButtonState == HIGH) {
        buzzerActive = false;
        digitalWrite(buzzerPin, LOW);
        delay(300); // debounce
      }
    }
  }

  lastButtonState = buttonState;
}

void updateDisplay()
{
  lcd.setCursor(0, 1);
  lcd.print("Count:         "); // clear old number
  lcd.setCursor(7, 1);
  if (settingTarget) {
    lcd.print(targetCount);
  } else {
    lcd.print(currentCount);
  }
}
