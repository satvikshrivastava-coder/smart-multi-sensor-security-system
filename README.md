# Smart Multi-Sensor Security & Environment Monitoring System

## Overview
This project is an Arduino-based smart home security and environmental monitoring system designed using multiple sensors and alert mechanisms. The system detects motion, vibration, environmental conditions, and abnormal activities using a combination of sensors and output devices.
## Project Preview

![Project Image](https://github.com/satvikshrivastava-coder/smart-multi-sensor-security-system/blob/main/Screenshot%202026-05-11%20173257.png)


## Features
- PIR motion detection
- MPU6050 vibration/tamper detection
- Temperature and humidity monitoring
- Light intensity monitoring
- LCD real-time display
- Buzzer alert system
- LED status indicators

## Components Used
- Arduino Uno
- PIR Sensor
- MPU6050 Accelerometer
- DHT22 Sensor
- LDR Sensor
- 16x2 I2C LCD
- Green LED
- Red LED
- Buzzer
- Breadboard
- Jumper Wires

## Working Principle
The system continuously monitors motion, vibration, temperature, humidity, and light conditions. If motion or tampering is detected, the buzzer and red LED activate automatically while the LCD displays warning messages.
## Wokwi Simulation
[Open Simulation](https://wokwi.com/projects/452827451841438721)
## Applications
- Home Security
- Office Monitoring
- Restricted Area Protection
- Smart Environment Monitoring


## Firmware Architecture

- **Non-blocking timing** — All sensor polling and alert durations use `millis()`-based intervals instead of `delay()`, keeping the main loop responsive at all times
- **Finite State Machine** — System operates across three defined states: `NORMAL`, `ALERT`, and `COOLDOWN`, with explicit transition logic preventing false re-triggers
- **LDR Night Mode** — Light intensity reading from LDR dynamically adjusts PIR sensitivity threshold; motion alerts suppress automatically in low-light conditions to reduce false positives
- **MPU6050 tamper detection** — Accelerometer data is read over I2C and compared against a calibrated vibration threshold; sustained vibration triggers a separate tamper alert distinct from motion
- **Sensor abstraction** — DHT22 temperature/humidity, PIR, LDR, and MPU6050 are each polled on independent `millis()` intervals (500ms, 100ms, 1s, 200ms respectively), preventing one slow sensor from blocking others


## Future Improvements
- IoT cloud integration
- Mobile app notifications
- ESP32 WiFi support
- Camera integration
- AI-based anomaly detection

## Developed By
Satvik Shrivastava  
B.Voc. (IoT)  
Dayalbagh Educational Institute
