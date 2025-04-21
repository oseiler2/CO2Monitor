#pragma once

#include <globals.h>
#include <model.h>

namespace Battery {
  void init(Model* _model);
  void readVoltage();
  boolean batteryPresent();
  boolean usbPowerPresent();
  uint16_t getBatteryLevelInmV();
  uint8_t getBatteryLevelInPercent(uint16_t mV);
}
