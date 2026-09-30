Smart Plant Monitoring and Automatic Irrigation System Using Arduino
Project Overview

This project is an Arduino-based Smart Plant Monitoring and Automatic Irrigation System. It monitors soil moisture, tank water level, temperature, and humidity. When the soil becomes dry and sufficient water is available in the tank, the system automatically turns on the water pump through a relay. The pump turns off when the soil becomes sufficiently wet.

Features
Automatic irrigation based on soil moisture.
Soil moisture monitoring using an analog soil moisture sensor.
Tank water level monitoring with calibrated percentage conversion.
Temperature and humidity monitoring using a DHT11 sensor.
16×2 I2C LCD display for system information.
Automatic pump control through a relay module.
Low tank water protection.
Buzzer alert for low tank level and DHT sensor error.
Pump maximum runtime safety protection.
Tank percentage stabilization and filtering to reduce sensor noise.
Serial Monitor output for sensor readings and system status.
Hardware Components
Arduino UNO
Soil Moisture Sensor
Tank Water Level Sensor
DHT11 Temperature and Humidity Sensor
16×2 I2C LCD
5V Relay Module
Mini DC Water Pump
Buzzer
External Power Supply for the Pump
Jumper Wires and Breadboard
Water Container and Plant Pot
Water Tube
Pin Connections
Component	Arduino Pin
Soil Moisture Sensor	A0
Tank Water Level Sensor	A1
DHT11	D2
Relay Module	D7
Buzzer	D8
I2C LCD SDA	A4
I2C LCD SCL	A5
Main Thresholds
Soil ADC ≥ 650: Soil is considered dry and the pump can turn ON.
Soil ADC ≤ 450: Soil is considered wet and the pump turns OFF.
Tank water level ≤ 20%: Tank is considered low, so the pump remains OFF and the buzzer turns ON.
Maximum pump runtime: 10 seconds.
DHT reading skip period after pump starts: 15 seconds.
LCD screen rotation interval: 3 seconds.
Tank Water Level Calibration

The tank water level sensor uses calibrated ADC values to estimate the water level percentage.

ADC Value	Water Level
0	0%
480	20%
600	40%
680	60%
720	80%
750	100%

These calibration values may need to be changed according to the actual tank, sensor position, and sensor readings.

System Operation
The Arduino reads the soil moisture sensor and determines whether the soil is dry or wet.
The tank sensor reading is filtered to reduce noise and converted into a water-level percentage.
The DHT11 provides temperature and humidity readings.
If the soil is dry and the tank has more than 20% water, the relay turns the pump ON.
When the soil becomes wet, the pump turns OFF automatically.
If the tank level reaches 20% or below, the pump is turned OFF and the buzzer is activated.
If the pump runs continuously for 10 seconds without reaching the wet condition, the safety lock stops the pump.
The LCD displays soil moisture, pump status, temperature, humidity, and tank water level.
Sensor values and system status are also displayed in the Serial Monitor.
LCD Display

The LCD rotates between three information screens:

Moisture and Pump Status
Temperature and Humidity
Tank Water Level Percentage
Serial Monitor Output

Example:

Soil: 500 | Temp: 28.0 C | Hum: 68 % | TankADC: 600 | Tank: 40 % | Pump: OFF


Safety Notes
Do not power the water pump directly from the Arduino board.
Use a separate external power supply suitable for the pump voltage and current.
The Arduino controls only the relay; the relay switches the external pump power.
Keep water away from the Arduino, breadboard, and exposed electronic connections.
Required Arduino Libraries
Wire
LiquidCrystal_I2C
DHT sensor library
Project Purpose

The main purpose of this project is to reduce manual plant watering by automatically monitoring plant and environmental conditions and supplying water when required, while protecting the pump when the tank water level is too low.





