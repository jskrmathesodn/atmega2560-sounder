# Ultrasonic Fish Finder Dashboard

A real-time sonar-style visualisation dashboard for HC-SR04 ultrasonic sensor data.

## Features
- Live scrolling waterfall display using matplotlib
- Sonar-style distance visualisation
- Serial communication with ATmega2560

## Requirements
- pyserial
- numpy
- matplotlib

## Hardware
- Arduino Mega 2560
- HC-SR04 Ultrasonic Sensor

## Usage
1. Upload ultrasonic_distance.c to your board
2. Update COM port in sounder_plot.py
3. Run: py sounder_plot.py