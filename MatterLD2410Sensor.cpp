#include <sdkconfig.h>
#ifdef CONFIG_ESP_MATTER_ENABLE_DATA_MODEL

#include "MatterCustom.h"
#include <app/server/Server.h>
#include "MatterLD2410Sensor.h"

using namespace esp_matter;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

#define DEFAULT_BRIGHTNESS 64

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

  log_i("Occupancy Sensor Attr update callback: endpoint: %u, cluster: %u, attribute: %u, val: %u", endpoint_id, cluster_id, attribute_id, val->val.u32);
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

  // // Basic node configuration
  // deviceNode.
  // root_node::config_t node_config;
  // char node_label[33] = "NervousLD2410";
  // strcpy(node_config.basic_information.node_label, node_label);

  // // endpoint handles can be used to add/modify clusters.
  // endpoint_t *endpointSensor = root_node::create(node::get(), &node_config, ENDPOINT_FLAG_NONE, (void *)this);
  // if (endpointSensor == nullptr) {
  //   log_e("Failed to create node endpoint");
  //   return false;
  // }
  // setEndPointId(endpoint::get_id(endpointSensor));
  // log_i("Occupancy Sensor created with endpoint_id %d", getEndPointId());

  // Occupancy config
  occupancy_sensor::config_t occupancy_sensor_config;
  occupancy_sensor_config.occupancy_sensing.occupancy = _occupancyState;
  occupancy_sensor_config.occupancy_sensing.occupancy_sensor_type = OCCUPANCY_SENSOR_TYPE_PIR;
  occupancy_sensor_config.occupancy_sensing.occupancy_sensor_type_bitmap = occupancySensorTypeBitmap[OCCUPANCY_SENSOR_TYPE_PIR];

  occupancyState = _occupancyState;
  // esp_err_t esperrOcc = occupancy_sensor::add(endpointSensor, &occupancy_sensor_config);
  // log_i("Occupancy Sensor created with endpoint_id %d", getEndPointId());

  // endpoint handles can be used to add/modify clusters.
  endpoint_t *endpointSensor = occupancy_sensor::create(node::get(), &occupancy_sensor_config, ENDPOINT_FLAG_NONE, (void *)this);
  if (endpointSensor == nullptr) {
    log_e("Failed to create occupancy endpoint");
    return false;
  }
  setEndPointId(endpoint::get_id(endpointSensor));
  log_i("Occupancy Sensor created with endpoint_id %d", getEndPointId());

  // Illuminance config
  light_sensor::config_t illuminance_config;
  illuminance_config.illuminance_measurement.illuminance_measured_value = _illuminanceMeasuredValue;
  illuminance_config.illuminance_measurement.illuminance_min_measured_value = nullptr;
  illuminance_config.illuminance_measurement.illuminance_max_measured_value = nullptr;
  // Add illuminance config to the same endpoint as occupancy 
  // (not working when creating another endpoint)
  esp_err_t esperrIll = light_sensor::add(endpointSensor, &illuminance_config);
  if (esperrIll != ESP_OK) {
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
  log_i("Occupancy Sensor set to %s", _occupancyState ? "Occupied" : "Vacant");

  return true;
}

bool MatterLD2410Sensor::setIlluminance(uint16_t _illuminanceMeasuredValue) {
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

  if (illuminanceVal.val.u16 != _illuminanceMeasuredValue) {
    illuminanceVal.val.u16 = _illuminanceMeasuredValue;
    bool ret;
    ret = updateAttributeVal(IlluminanceMeasurement::Id, IlluminanceMeasurement::Attributes::MeasuredValue::Id, &illuminanceVal);
    if (!ret) {
      log_e("Failed to update Illuminance Sensor Attribute.");
      return false;
    }
    illuminanceMeasuredValue = _illuminanceMeasuredValue;
  }
  log_d("Illuminance Sensor set to %0.2f", illuminanceVal.val.u16);

  return true;
}

#endif /* CONFIG_ESP_MATTER_ENABLE_DATA_MODEL */
