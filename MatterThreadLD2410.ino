/*
 * This code will create a Matter Device which can be
 * commissioned and controlled from a Matter Environment APP.
 * Additionally the ESP32 will send debug messages indicating the Matter activity.
 * Turning DEBUG Level ON may be useful to following Matter Accessory and Controller messages.
 *
 * This will create a Matter LD2410 Sensor Device over Thread network.
 * It features :
 * - light (onboard LED)
 * - occupancy detection
 * - illuminance
 *
 * The onboard button can be kept pressed for 5 seconds to decommission the Matter Node.
 *
 */

// Matter Manager
#include "Thread.h"
#include "MatterCustom.h"
#include "LD2410.h"
#include <elapsedMillis.h>
#include <Preferences.h>
#include "config.h"

// List of Matter Endpoints for this Node
// Matter LD2410 Sensor Endpoint
MatterLD2410Sensor LD2410Sensor;
// Color Light Endpoint
MatterColorLight ColorLight;


// it will keep last OnOff & HSV Color state stored, using Preferences
Preferences matterPref;
const char *onOffPrefKey = "OnOff";
const char *hsvColorPrefKey = "HSV";

// set your board RGB LED pin here
const uint8_t ledPin = 8;  // Set your pin here if your board has not defined LED_BUILTIN

// set your board USER BUTTON pin here - decommissioning only
const uint8_t buttonPin = BOOT_PIN;  // Set your pin here. Using BOOT Button.

// Button control
uint32_t button_time_stamp = 0;                // debouncing control
bool button_state = false;                     // false = released | true = pressed
const uint32_t decommissioningTimeout = 5000;  // keep the button pressed for 5s, or longer, to decommission

elapsedMillis CHECK_OCCUPANCY_SENSOR = 0;

// bool MatterLD2410Sensor::attributeChangeCB(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val) {
//   bool ret = true;
//   if (!started) {
//     Serial.printf("Matter Occupancy Sensor device has not begun.");
//     return false;
//   }

//   Serial.printf("\nOccupancy Sensor Attr update callback: endpoint: %u, cluster: %u, attribute: %u, val: %u", endpoint_id, cluster_id, attribute_id, val->val.u32);
//   return ret;
// }

// Set the RGB LED Light based on the current state of the Color Light
bool setLightState(bool state, espHsvColor_t colorHSV) {

  if (state) {
#ifdef RGB_BUILTIN
    espRgbColor_t rgbColor = espHsvColorToRgbColor(colorHSV);
    // set the RGB LED
    rgbLedWrite(ledPin, rgbColor.r, rgbColor.g, rgbColor.b);
#else
    // No Color RGB LED, just use the HSV value (brightness) to control the LED
    analogWrite(ledPin, colorHSV.v);
#endif
  } else {
#ifndef RGB_BUILTIN
    // after analogWrite(), it is necessary to set the GPIO to digital mode first
    pinMode(ledPin, OUTPUT);
#endif
    digitalWrite(ledPin, LOW);
  }
  // store last HSV Color and OnOff state for when the Light is restarted / power goes off
  matterPref.putBool(onOffPrefKey, state);
  matterPref.putUInt(hsvColorPrefKey, colorHSV.h << 16 | colorHSV.s << 8 | colorHSV.v);
  // This callback must return the success state to Matter core
  return true;
}

void setup() {
  #undef CHIP_DEVICE_CONFIG_DEVICE_VENDOR_NAME
  #define CHIP_DEVICE_CONFIG_DEVICE_VENDOR_NAME "NervousInc"
  // Initialize the USER BUTTON (Boot button) that will be used to decommission the Matter Node
  pinMode(buttonPin, INPUT_PULLUP);
  // Initialize the LED (light) GPIO and Matter End Point
  pinMode(ledPin, OUTPUT);

  Serial.begin(115200);

  Serial.println("Setting all up !");

  // Thread setup
  setupThread();
  waitThread();

  // set initial LD2410 sensor state as false and connected to a PIR sensor type (default)
  LD2410Sensor.begin();

  // Initialize Matter EndPoint
  matterPref.begin("MatterPrefs", false);
  // default OnOff state is ON if not stored before
  bool lastOnOffState = matterPref.getBool(onOffPrefKey, true);
  // default HSV color is blue HSV(169, 254, 254)
  uint32_t prefHsvColor = matterPref.getUInt(hsvColorPrefKey, 169 << 16 | 254 << 8 | 254);
  espHsvColor_t lastHsvColor = { uint8_t(prefHsvColor >> 16), uint8_t(prefHsvColor >> 8), uint8_t(prefHsvColor) };
  ColorLight.begin(lastOnOffState, lastHsvColor);
  // set the callback function to handle the Light state change
  ColorLight.onChange(setLightState);

  // lambda functions are used to set the attribute change callbacks
  ColorLight.onChangeOnOff([](bool state) {
    Serial.printf("Light OnOff changed to %s\r\n", state ? "ON" : "OFF");
    return true;
  });
  ColorLight.onChangeColorHSV([](HsvColor_t hsvColor) {
    Serial.printf("Light HSV Color changed to (%d,%d,%d)\r\n", hsvColor.h, hsvColor.s, hsvColor.v);
    return true;
  });

  // Matter beginning - Last step, after all EndPoints are initialized
  MatterCustom.begin();
  // Matter.set_vendor_name("NervousInc");


  // Check Matter Accessory Commissioning state, which may change during execution of loop()
  if (!MatterCustom.isDeviceCommissioned()) {
    Serial.println("");
    Serial.println("Matter Node is not commissioned yet.");
    Serial.println("Initiate the device discovery in your Matter environment.");
    Serial.println("Commission it to your Matter hub with the manual pairing code or QR code");
    Serial.printf("Manual pairing code: %s\r\n", MatterCustom.getManualPairingCode().c_str());
    Serial.printf("QR code URL: %s\r\n", MatterCustom.getOnboardingQRCodeUrl().c_str());
    // waits for Matter Occupancy Sensor Commissioning.
    uint32_t timeCount = 0;
    while (!MatterCustom.isDeviceCommissioned()) {
      delay(1000);
      if ((timeCount++ % 50) == 0) {  // 50*100ms = 5 sec
        Serial.println("Matter Node not commissioned yet. Waiting for commissioning.");
      }
    }
    Serial.println("Matter Node is commissioned and connected to Thread. Ready for use.");
    Serial.printf(
      "Initial state: %s | RGB Color: (%d,%d,%d) \r\n", ColorLight ? "ON" : "OFF", ColorLight.getColorRGB().r, ColorLight.getColorRGB().g,
      ColorLight.getColorRGB().b);
    // configure the Light based on initial on-off state and its color
    ColorLight.updateAccessory();
  }

  // Setup LD2410 sensor
  setupLD2410();
}

void loop() {
  loopLD2410();

  if(CHECK_OCCUPANCY_SENSOR >= 50) {
    // Check if the button has been pressed
    if (digitalRead(buttonPin) == LOW && !button_state) {
      // deals with button debouncing
      button_time_stamp = millis();  // record the time while the button is pressed.
      button_state = true;           // pressed.
    }

    if (button_state && digitalRead(buttonPin) == HIGH) {
      button_state = false;  // released
    }

    // Onboard User Button is kept pressed for longer than 5 seconds in order to decommission matter node
    uint32_t time_diff = millis() - button_time_stamp;
    if (button_state && time_diff > decommissioningTimeout) {
      Serial.println("Decommissioning Occupancy Sensor Matter Accessory. It shall be commissioned again.");
      Matter.decommission();
      button_time_stamp = millis();  // avoid running decommissining again, reboot takes a second or so
    }

    // Check Occupancy Sensor and set Matter Attribute
    if(getPresenceDetected() != LD2410Sensor.getOccupancy()){
      Serial.print("Occupancy Sensor : ");
      Serial.println(getPresenceDetected());
      LD2410Sensor.setOccupancy(getPresenceDetected());
    }
    // Check Illuminance Sensor and set Matter Attribute
    if(getLightLevel() != LD2410Sensor.getIlluminance()) {
      Serial.print("Light level : ");
      Serial.println(getLightLevel());
      LD2410Sensor.setIlluminance((float)getLightLevel()*100.00);
    }

    CHECK_OCCUPANCY_SENSOR = 0;
  }
}
