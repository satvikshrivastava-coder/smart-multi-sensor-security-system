# Smart Multi-Sensor Security & Environment Monitoring System

Arduino Uno firmware (C++, Arduino framework) that watches for motion and tampering while monitoring temperature, humidity and ambient light. Outputs go to a 16x2 I2C LCD, two LEDs, a buzzer and the serial monitor.

**Status:** simulation only (Wokwi). Not yet tested on physical hardware.

**Live simulation:** [Open in Wokwi](https://wokwi.com/projects/452827451841438721)

## Preview

| Safe state | Alert state |
|---|---|
| ![Safe state](model%20preview%20%3A%20when%20safe.png) | ![Alert state](model%20preview%20%3A%20when%20alert.png) |

| Circuit diagram | Serial monitor |
|---|---|
| ![Circuit diagram](circuit%20diagram.png) | ![Serial monitor output](serial%20monitor%20with%20sensor%20reading.png) |

## Hardware

| Component | Pin / Interface | Purpose |
|---|---|---|
| Arduino Uno (ATmega328P) | - | Microcontroller |
| PIR sensor | D2 (INT0) | Motion detection |
| DHT22 | D7 | Temperature and humidity |
| LDR | A0 | Ambient light level |
| MPU6050 | I2C (A4 SDA, A5 SCL) | Acceleration, used for tamper detection |
| 16x2 LCD with I2C backpack | I2C, address 0x27 | Status display |
| Green LED | D9 | Normal status |
| Red LED | D8 | Alert status |
| Buzzer | D4 | Audible alert |

## Libraries

- LiquidCrystal_I2C
- DHT sensor library
- Adafruit MPU6050
- Adafruit Unified Sensor

## How it works

The firmware is a finite state machine with five states. One state is chosen per sensor cycle, and each state defines its own outputs.

| State | Condition | Outputs |
|---|---|---|
| `SAFE` | No motion, no tamper, light level at or above 500 | Green LED on. LCD shows temperature and humidity |
| `NIGHT_MODE` | No motion, no tamper, light level below 500 | All LEDs and buzzer off. LCD shows temperature and humidity. Detection stays active |
| `MOTION_ALERT` | PIR output high | Red LED and buzzer on. LCD shows `MOTION ALERT!` |
| `TAMPER_ALERT` | MPU6050 X or Y acceleration at or above 1.0 g | Red LED and buzzer on. LCD shows `TAMPER ALERT!` |
| `MOTION_TAMPER_ALERT` | Both conditions at once | Red LED and buzzer on. LCD shows `MOTION+TAMPER!` |

Alert states take priority over `NIGHT_MODE` and `SAFE`.

## Firmware design

- **Non-blocking timing.** Sensors are read once per second using `millis()`. The main loop never calls `delay()`. The only delays are in `setup()` for the start-up screens.
- **PIR hardware interrupt.** A rising edge on D2 runs an interrupt service routine that sets a `volatile` flag. The main loop reacts to the flag immediately without waiting for the next sensor cycle.
- **Sensor validation.** If the DHT22 returns an invalid reading (NaN), that cycle is skipped.
- **Flicker-free LCD.** `updateLCD()` compares the new text with what is already displayed and redraws only when it changed.
- **State logic separated from outputs.** The sensor logic decides the state. `applyState()` drives the LEDs, buzzer and LCD for that state.

## Configuration

| Constant | Value | Meaning |
|---|---|---|
| `MOTION_THRESHOLD` | 1.0 | Tamper threshold in g, applied to the X and Y axes |
| `DARK_THRESHOLD` | 500 | LDR reading (0-1023) below which `NIGHT_MODE` is used |
| `SENSOR_INTERVAL` | 1000 | Time between sensor cycles, in ms |

The MPU6050 is set to the +-2 g range with a 21 Hz filter bandwidth.

## Running it

**Wokwi:** open the simulation link above and press play.

**Arduino IDE:** install the four libraries above, select Arduino Uno, then open and upload `smart_security_system.ino`.

The serial monitor runs at 9600 baud and prints the state and all sensor values once per cycle:

```
State: SAFE | Light: 812 | PIR: 0 | Temp: 24.00 | Hum: 40.00 | AX: 0.01 | AY: 0.02
```

## Known limitations and next steps

- Simulation only. Real sensor noise, PIR warm-up time and wiring are untested.
- A failed DHT22 read skips the whole sensor cycle, so alert states are not re-evaluated during that cycle.
- The LCD code uses `String` objects, which is not ideal on a board with 2 KB of RAM. Fixed character buffers would be safer.
- Thresholds are fixed constants with no calibration, and the PIR input has no debouncing.
- Planned: port to an STM32 board with register-level GPIO, add a watchdog timer, and replace `String` with character buffers.

## Author

Satvik Shrivastava
B.Voc IoT, Dayalbagh Educational Institute, Agra
