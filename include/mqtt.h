#ifndef _MQTT_H
#define _MQTT_H

#include "globals.h"
#include <model.h>

namespace mqtt {
  typedef void (*calibrateCo2SensorCallback_t)(uint16_t);

  void setupMqtt(Model* _model, calibrateCo2SensorCallback_t calibrateCo2SensorCallback);

  void publishSensors();
  void publishConfiguration();

  void mqttLoop(void* pvParameters);

  extern TaskHandle_t mqttTask;
}

#endif