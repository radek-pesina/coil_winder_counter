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
unsigned long lastButtonActivityTime = 0;  // for tracking inactivity period
const unsigned long LONG_PRESS_TIME = 2000;  // 2 seconds for acceleration
const unsigned long INACTIVITY_TIME = 5000;  // 5 seconds of no button activity before starting
const unsigned long FAST_INCREMENT_INTERVAL = 200;  // 5 times per second (1000/5=200ms)
const uint8_t PULSES_PER_DECREMENT = 1; // number of sensor pulses per coil decrement
unsigned long lastFastIncrementTime = 0;
// Prevent multiple decrements when stopped on sensor or noisy signal
const unsigned long MIN_DECREMENT_INTERVAL = 300; // ms; ignore decrements faster than this
unsigned long lastDecrementTime = 0;
// Sensor priming flag (cleared when returning to target-set mode)
bool sensorPrimed = false;

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

  if (settingTarget)
  {
    // Target setting mode
    if (buttonState == LOW && lastButtonState == HIGH)
    {
      // Button just pressed
      buttonPressStartTime = millis();
      lastButtonActivityTime = millis();
      targetCount++;
      updateDisplay();
      delay(50); // Initial debounce
    }
    else if (buttonState == LOW && (millis() - buttonPressStartTime) > LONG_PRESS_TIME)
    {
      // Long press - fast increment
      if (millis() - lastFastIncrementTime >= FAST_INCREMENT_INTERVAL)
      {
        targetCount += 5;
        updateDisplay();
        lastFastIncrementTime = millis();
        lastButtonActivityTime = millis(); // Update activity time during fast increment
      }
    }
    else if (buttonState == HIGH && lastButtonState == LOW)
    {
      // Button released
      lastButtonActivityTime = millis(); // Update activity time on release
      if (millis() - buttonPressStartTime > LONG_PRESS_TIME)
      {
        delay(500); // Additional delay after fast increment
      }
    }

    // Exit target setting mode after INACTIVITY_TIME of no button activity
    if (buttonState == HIGH && targetCount > 0 &&
        (millis() - lastButtonActivityTime) > INACTIVITY_TIME)
        {
      settingTarget = false;
      currentCount = targetCount;
      lcd.clear();
      lcd.print("Winding:");
      updateDisplay();
      printf("Target set: %lu\n", targetCount);
    }
  }
  else
  {
    // Normal counting mode
    // Simpler falling-edge based pulse-pair detector (works with your sensor: steady HIGH, two flashes per pass)
    static unsigned long lastSensorTriggerTime = 0;
    static uint8_t pulseCount = 0;  // Count falling-edge pulses before decrementing
    const unsigned long sensorDebounceTime = 50; // ms debounce for edges
    int sensorState = digitalRead(sensorPin);
    unsigned long currentTime = millis();

    // Detect falling edge (HIGH -> LOW)
    if (sensorState == LOW && lastSensorState == HIGH)
    {
      pulseCount++;
      lastSensorTriggerTime = currentTime;
      printf("Pulse detected (count=%u)\n", pulseCount);

      if (pulseCount >= PULSES_PER_DECREMENT)
      {
        // PULSES_PER_DECREMENT falling edges = one magnet pass
        if (currentTime - lastDecrementTime > MIN_DECREMENT_INTERVAL)
        {
          if (currentCount > 0)
          {
            currentCount--;
            updateDisplay();
            printf("Coils remaining: %lu (pulse pair)\n", currentCount);
            if (currentCount == 0)
            {
              buzzerActive = true;
            }
          }
          lastDecrementTime = currentTime;
        }
        else
        {
          printf("Ignored pulse pair (too fast): %lums since last decrement\n", currentTime - lastDecrementTime);
        }
        pulseCount = 0;
      }
    }
    lastSensorState = sensorState;

    // Reset pulse counter if pulses are too far apart (prevent stale partial counts)
    if (currentTime - lastSensorTriggerTime > 2000)
    {
      pulseCount = 0;
    }

    // Handle buzzer and reset functionality
    if (buzzerActive)
    {
      // Use tone() for a continuous audible buzz at 1kHz
      tone(buzzerPin, 1000); // 1kHz

      // Check for button press to stop buzzer and reset device
      if (buttonState == LOW && lastButtonState == HIGH)
      {
        // Stop buzzer
        buzzerActive = false;
        noTone(buzzerPin);

        // Reset to initial state
        settingTarget = true;
        targetCount = 0;
        currentCount = 0;
        lastButtonActivityTime = millis();
        sensorPrimed = false; // ensure sensor timing will be re-initialized for the next winding

        // Reset display to initial state
        lcd.clear();
        lcd.print("Set Target:");
        updateDisplay();

        printf("Device reset for new target\n");
        delay(300); // debounce
      }
    } else {
      noTone(buzzerPin); // Ensure buzzer is off when not active
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
