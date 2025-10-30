# Coil Winder (Hall-sensor counter)

Simple coil winder counter using a 49E Hall effect sensor and an LCD. The device lets you set a target number of turns, then counts down as the coil is wound. When the target is reached the buzzer beeps; pressing the button will silence the buzzer.

## Features

- Set target count on power-up using a single button (D2)
- Single press increments by 1
- Press-and-hold for 2 seconds enables accelerated incrementing at 5 increments per second
- After 3 seconds of no button activity, the system starts winding and counts down
- Hall-effect sensor (D12) detects rotations and decrements the remaining count
- Buzzer (D13) beeps when count reaches zero; pressing the button silences it
- 16x2 LCD display for status and numeric feedback (LiquidCrystal library)
- Serial logging at 9600 baud for debugging

## Pinout (as used in `project.ino`)

- LCD (16x2, 4-bit mode)
  - RS -> D7
  - EN -> D6
  - D4 -> D5
  - D5 -> D4
  - D6 -> D3
  - D7 -> D8
  - VSS -> GND
  - VDD -> 5V
  - VO -> middle of 10k pot (contrast)
  - Backlight + -> 5V (with resistor) ; Backlight − -> GND

- Hall sensor (49E)
  - DO -> D12
  - VCC -> 5V
  - GND -> GND
  - Note: `pinMode(sensorPin, INPUT_PULLUP)` is enabled in code so sensor output should be open/low-active or pulled low when magnet is detected.

- User button
  - One leg -> D2
  - Other leg -> GND
  - Internal pull-up is used in code (`INPUT_PULLUP`) so the button should connect the pin to GND when pressed.

- Buzzer
  - + -> D13 (through a small series resistor if using an active piezo or if the buzzer has no built-in resistor)
  - − -> GND

## Wiring notes and safety

- Use the 10k pot on LCD VO pin to adjust contrast.
- If your buzzer draws significant current, drive it with a transistor (NPN) and use a flyback diode if it's inductive. The code drives D13 directly which is OK for small passive/active buzzers but not for motors or large speakers.
- Ensure common GND between Arduino, sensor, LCD, and buzzer.

## Behaviour / How to use

1. Power up the device. The LCD will show `Set Target:` and the second line `Count: 0`.
2. Set the target number of turns using the button (D2):
   - Each short press increments the target by +1.
   - Press and hold for 2 seconds: the code enters accelerated mode adding +5 every 200ms (i.e., 5 per second).
3. After you stop pressing the button and there is no button activity for ~3 seconds, the device leaves "set target" mode and begins the winding/countdown phase. The LCD will show `Winding:` and the remaining count.
4. As the coil is turned, the Hall sensor (D12) edges will decrement the remaining count by 1 each. Serial messages are printed at 9600 baud for debugging (e.g., `Coils remaining: <n>`).
5. When the count reaches zero:
   - A beep pattern begins (500ms on, 500ms off) driven on D13.
   - Pressing the button once will stop the buzzer.

## Code / Libraries

- The sketch uses the standard `LiquidCrystal` library (bundled with the Arduino IDE).
- Serial logging uses `Serial.begin(9600)` and printf support is enabled in the sketch.

## Uploading

1. Open the Arduino IDE (or PlatformIO) and select the correct board (Arduino Uno) and COM port.
2. Open `project\project.ino` in the IDE and upload.
3. Open Serial Monitor at 9600 baud if you want to see debug messages.

## Troubleshooting

- If increments don't register when winding, check that the Hall sensor is wired to D12, GND, and 5V and that the sensor is positioned close enough to the magnet/rotor.
- If the button behaves inverted, verify the wiring: button should pull D2 to GND when pressed (internal pull-up is used).
- If the buzzer is silent when expected, check wiring and consider using a transistor driver if your buzzer needs more current than an I/O pin can supply.

## Suggested improvements / notes

- Add a small LED to indicate "target set" vs "winding" states visually.
- Add persistent storage (EEPROM) if you want to remember the last target across power cycles.
- Consider hardware debouncing (RC) or more robust software debouncing if mechanical bouncing causes multiple increments.
