// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <sdkconfig.h>
#ifdef CONFIG_ESP_MATTER_ENABLE_DATA_MODEL

#include "MatterCustom.h"
#include <app/server/Server.h>
#include "MatterLD2410Sensor.h"

using namespace esp_matter;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

// clang-format off
const uint8_t MatterLD2410Sensor::occupancySensorTypeBitmap[4] = {
  MatterLD2410Sensor::occupancySensorTypePir,
  MatterLD2410Sensor::occupancySensorTypePir | MatterLD2410Sensor::occupancySensorTypeUltrasonic,
  MatterLD2410Sensor::occupancySensorTypeUltrasonic,
  MatterLD2410Sensor::occupancySensorTypePhysicalContact
};
// clang-format on

bool MatterLD2410Sensor::attributeChangeCB(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val) {
  bool ret = true;
  if (!started) {
    log_e("Matter Occupancy Sensor device has not begun.");
    return false;
  }

  log_d("Occupancy Sensor Attr update callback: endpoint: %u, cluster: %u, attribute: %u, val: %u", endpoint_id, cluster_id, attribute_id, val->val.u32);
  return ret;
}

MatterLD2410Sensor::MatterLD2410Sensor() {}

MatterLD2410Sensor::~MatterLD2410Sensor() {
  end();
}

bool MatterLD2410Sensor::begin(bool _occupancyState, byte _illuminanceMeasuredValue) {
  ArduinoMatterCustom::_init();

  if (getEndPointId() != 0) {
    log_e("Matter Occupancy Sensor with Endpoint Id %d device has already been created.", getEndPointId());
    return false;
  }

  // Occupancy config
  occupancy_sensor::config_t occupancy_sensor_config;
  occupancy_sensor_config.occupancy_sensing.occupancy = _occupancyState;
  occupancy_sensor_config.occupancy_sensing.occupancy_sensor_type = OCCUPANCY_SENSOR_TYPE_PIR;
  occupancy_sensor_config.occupancy_sensing.occupancy_sensor_type_bitmap = occupancySensorTypeBitmap[OCCUPANCY_SENSOR_TYPE_PIR];

  // endpoint handles can be used to add/modify clusters.
  endpoint_t *endpointSensor = occupancy_sensor::create(node::get(), &occupancy_sensor_config, ENDPOINT_FLAG_NONE, (void *)this);
  if (endpointSensor == nullptr) {
    log_e("Failed to create Occupancy Sensor endpoint");
    return false;
  }
  occupancyState = _occupancyState;
  setEndPointId(endpoint::get_id(endpointSensor));
  log_i("Occupancy Sensor created with endpoint_id %d", getEndPointId());

  // Illuminance config
  unsigned short int illuminance_min_measured_value = 0;
  unsigned short int illuminance_max_measured_value = 255;

  light_sensor::config_t illuminance_config;
  illuminance_config.illuminance_measurement.illuminance_measured_value = _illuminanceMeasuredValue;
  illuminance_config.illuminance_measurement.illuminance_min_measured_value = illuminance_min_measured_value;
  illuminance_config.illuminance_measurement.illuminance_max_measured_value = illuminance_max_measured_value;

  // Add illuminance config to the same endpoint as occupancy 
  // (not working when creating another endpoint)
  esp_err_t esperr = light_sensor::add(endpointSensor, &illuminance_config);
  if (esperr != ESP_OK) {
    log_e("Failed to add light sensor to endpoint");
    return false;
  }
  illuminanceMeasuredValue = _illuminanceMeasuredValue;
  log_i("Light sensor created with endpoint_id %d", getEndPointId());

  started = true;
  return true;
}

void MatterLD2410Sensor::end() {
  started = false;
}

bool MatterLD2410Sensor::setOccupancy(bool _occupancyState) {
  if (!started) {
    log_e("Matter Occupancy Sensor device has not begun.");
    return false;
  }

  // avoid processing if there was no change
  if (occupancyState == _occupancyState) {
    return true;
  }

  esp_matter_attr_val_t occupancyVal = esp_matter_invalid(NULL);

  if (!getAttributeVal(OccupancySensing::Id, OccupancySensing::Attributes::Occupancy::Id, &occupancyVal)) {
    log_e("Failed to get Occupancy Sensor Attribute.");
    return false;
  }
  if (occupancyVal.val.u8 != _occupancyState) {
    occupancyVal.val.u8 = _occupancyState;
    bool ret;
    ret = updateAttributeVal(OccupancySensing::Id, OccupancySensing::Attributes::Occupancy::Id, &occupancyVal);
    if (!ret) {
      log_e("Failed to update Occupancy Sensor Attribute.");
      return false;
    }
    occupancyState = _occupancyState;
  }
  log_v("Occupancy Sensor set to %s", _occupancyState ? "Occupied" : "Vacant");

  return true;
}

bool MatterLD2410Sensor::setIlluminance(byte _illuminanceMeasuredValue) {
  if (!started) {
    log_e("Matter Illuminance Sensor device has not begun.");
    return false;
  }

  // avoid processing if there was no change
  if (illuminanceMeasuredValue == _illuminanceMeasuredValue) {
    return true;
  }

  esp_matter_attr_val_t illuminanceVal = esp_matter_invalid(NULL);

  if (!getAttributeVal(IlluminanceMeasurement::Id, IlluminanceMeasurement::Attributes::MeasuredValue::Id, &illuminanceVal)) {
    log_e("Failed to get Illuminance Sensor Attribute.");
    return false;
  }
  if (illuminanceVal.val.u8 != _illuminanceMeasuredValue) {
    illuminanceVal.val.u8 = _illuminanceMeasuredValue;
    bool ret;
    ret = updateAttributeVal(IlluminanceMeasurement::Id, IlluminanceMeasurement::Attributes::MeasuredValue::Id, &illuminanceVal);
    if (!ret) {
      log_e("Failed to update Illuminance Sensor Attribute.");
      return false;
    }
    illuminanceMeasuredValue = _illuminanceMeasuredValue;
  }
  log_v("Illuminance Sensor set to %d", _illuminanceMeasuredValue);

  return true;
}

#endif /* CONFIG_ESP_MATTER_ENABLE_DATA_MODEL */
