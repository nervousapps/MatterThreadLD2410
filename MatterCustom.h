#pragma once
#ifdef CONFIG_ESP_MATTER_ENABLE_DATA_MODEL

#include <Matter.h>

#include "MatterLD2410Sensor.h"

using namespace esp_matter;

class ArduinoMatterCustom: public ArduinoMatter {
public:
  // list of custom Matter EndPoints Friend Classes
  friend class MatterLD2410Sensor;
};

extern ArduinoMatterCustom MatterCustom;

#endif /* CONFIG_ESP_MATTER_ENABLE_DATA_MODEL */
