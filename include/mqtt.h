#ifndef _MQTT_H
#define _MQTT_H

#include "globals.h"
#include <model.h>

namespace mqtt {
  typedef void (*calibrateCo2SensorCallback_t)(uint16_t);
  typedef void (*setTemperatureOffsetCallback_t)(float);
  typedef float (*getTemperatureOffsetCallback_t)(void);

  void setupMqtt(Model* _model, calibrateCo2SensorCallback_t calibrateCo2SensorCallback, setTemperatureOffsetCallback_t _setTemperatureOffsetCallback, getTemperatureOffsetCallback_t _getTemperatureOffsetCallback);

  void publishSensors(uint16_t mask);
  void publishConfiguration();

  void mqttLoop(void* pvParameters);

  extern TaskHandle_t mqttTask;
}

#endif