/*
  This program reads all data received from
  the HLK-LD2410 presence sensor and periodically
  prints the values to the serial monitor.
  
  Several #defines control the behavior of the program:
  #define SERIAL_BAUD_RATE sets the serial monitor baud rate
  #define ENHANCED_MODE enables the enhanced (engineering)
  mode of the sensor. Comment that line to switch to basic mode.
  #define DEBUG_MODE enables the printing of debug information
  (all reaceived frames are printed). Comment the line to disable
  debugging.

  Communication with the sensor is handled by the 
  "MyLD2410" library Copyright (c) Iavor Veltchev 2024

  Use only hardware UART at the default baud rate 256000,
  or change the #define LD2410_BAUD_RATE to match your sensor.
  For ESP32 or other boards that allow dynamic UART pins,
  modify the RX_PIN and TX_PIN defines

  Connection diagram:
  Arduino/ESP32 RX  -- TX LD2410 
  Arduino/ESP32 TX  -- RX LD2410
  Arduino/ESP32 GND -- GND LD2410
  Provide sufficient power to the sensor Vcc (200mA, 5-12V) 
*/
#define sensorSerial Serial1

// User defines
// #define DEBUG_MODE
#define ENHANCED_MODE
#define SERIAL_BAUD_RATE 115200

//Change the communication baud rate here, if necessary
#define LD2410_BAUD_RATE 256000
#include "MyLD2410.h"
#include "config.h"

#ifdef DEBUG_MODE
MyLD2410 sensor(sensorSerial, true);
#else
MyLD2410 sensor(sensorSerial);
#endif

bool presenceDetected = false;
byte lightLevel = 0;

unsigned long nextPrint = 0, printEvery = 10;  // print every second

void setupLD2410() {
  sensorSerial.begin(LD2410_BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(2000);
  Serial.println(__FILE__);
  if (!sensor.begin()) {
    Serial.println("Failed to communicate with the sensor.");
    while (true) {}
  }

#ifdef ENHANCED_MODE
  sensor.enhancedMode();
#else
  sensor.enhancedMode(false);
#endif

sensor.setNoOneWindow(5);

  delay(nextPrint);
}

bool getPresenceDetected() {
  return presenceDetected;
}

byte getLightLevel() {
  return lightLevel;
}

bool setRadarTimeout(byte timeout) {
  return sensor.setNoOneWindow(timeout);
}

void loopLD2410() {
  if ((sensor.check() == MyLD2410::Response::DATA) && (millis() > nextPrint)) {
    nextPrint = millis() + printEvery;
    if (sensor.presenceDetected()) {
      presenceDetected = true;
    } else {
      presenceDetected = false;
    }
    lightLevel = sensor.getLightLevel();
  }
}