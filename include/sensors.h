#ifndef _SENSORS_H
#define _SENSORS_H

#include <globals.h>

#include <scd30.h>
#include <scd40.h>
#include <sps_30.h>
#include <bme680.h>

namespace Sensors {

  void setupSensorsLoop(SCD30* scd30, SCD40* scd40, SPS_30* sps30, BME680* bme680);

  TaskHandle_t start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core);

  void sensorsLoop(void* pvParameters);

  extern TaskHandle_t sensorsTask;

}

#endif