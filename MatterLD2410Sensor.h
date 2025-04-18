#pragma once
#include <sdkconfig.h>
#ifdef CONFIG_ESP_MATTER_ENABLE_DATA_MODEL

#include "MatterCustom.h"
#include <MatterEndPoint.h>
#include <app-common/zap-generated/cluster-objects.h>

using namespace chip::app::Clusters::BasicInformation;
using namespace chip::app::Clusters::OccupancySensing;
using namespace chip::app::Clusters::IlluminanceMeasurement;

class MatterLD2410Sensor : public MatterEndPoint {
public:
  // Different Occupancy Sensor Types
  enum OccupancySensorType_t {
    OCCUPANCY_SENSOR_TYPE_PIR = (uint8_t)OccupancySensorTypeEnum::kPir,
    OCCUPANCY_SENSOR_TYPE_ULTRASONIC = (uint8_t)OccupancySensorTypeEnum::kUltrasonic,
    OCCUPANCY_SENSOR_TYPE_PIR_AND_ULTRASONIC = (uint8_t)OccupancySensorTypeEnum::kPIRAndUltrasonic,
    OCCUPANCY_SENSOR_TYPE_PHYSICAL_CONTACT = (uint8_t)OccupancySensorTypeEnum::kPhysicalContact
  };

  MatterLD2410Sensor();
  ~MatterLD2410Sensor();
  // begin Matter Occupancy Sensor endpoint with initial occupancy state and default PIR sensor type
  bool begin(bool _occupancyState = false, byte _illuminanceMeasuredValue = 0);
  // this will just stop processing Occupancy Sensor Matter events
  void end();

  // set the occupancy state
  bool setOccupancy(bool _occupancyState);
  // returns the occupancy state
  bool getOccupancy() {
    return occupancyState;
  }

  // bool conversion operator
  void operator=(bool _occupancyState) {
    setOccupancy(_occupancyState);
  }
  // bool conversion operator
  operator bool() {
    return getOccupancy();
  }

  // set the illuminance state
  bool setIlluminance(uint16_t _illuminanceMeasuredValue);
  uint16_t getIlluminance() {
    return illuminanceMeasuredValue;
  }

  // bool conversion operator
  void operator=(uint16_t _illuminanceMeasuredValue) {
    setIlluminance(_illuminanceMeasuredValue);
  }
  // byte conversion operator
  operator uint16_t() {
    return getIlluminance();
  }

  // this function is called by Matter internal event processor. It could be overwritten by the application, if necessary.
  bool attributeChangeCB(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val);

protected:
  // bitmap for Occupancy Sensor Types
  static const uint8_t occupancySensorTypePir = 0x01;
  static const uint8_t occupancySensorTypeUltrasonic = 0x02;
  static const uint8_t occupancySensorTypePhysicalContact = 0x04;

  // bitmap for Occupancy Sensor Type Bitmap mapped array
  static const uint8_t occupancySensorTypeBitmap[4];

  bool started = false;
  bool occupancyState = false;

  uint16_t illuminanceMeasuredValue = 0;
};
#endif /* CONFIG_ESP_MATTER_ENABLE_DATA_MODEL */
