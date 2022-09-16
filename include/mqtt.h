#ifndef _MQTT_H
#define _MQTT_H

#include "globals.h"
#include <model.h>

namespace mqtt {
  typedef void (*calibrateCo2SensorCallback_t)(uint16_t);
  typedef void (*setTemperatureOffsetCallback_t)(float);
  typedef float (*getTemperatureOffsetCallback_t)(void);
  typedef uint32_t(*getSPS30AutoCleanIntervalCallback_t)(void);
  typedef boolean(*setSPS30AutoCleanIntervalCallback_t)(uint32_t);
  typedef boolean(*cleanSPS30Callback_t)(void);
  typedef uint8_t(*getSPS30StatusCallback_t)(void);

  void setupMqtt(
    Model* model,
    calibrateCo2SensorCallback_t calibrateCo2SensorCallback,
    setTemperatureOffsetCallback_t setTemperatureOffsetCallback,
    getTemperatureOffsetCallback_t getTemperatureOffsetCallback,
    getSPS30AutoCleanIntervalCallback_t getSPS30AutoCleanIntervalCallback,
    setSPS30AutoCleanIntervalCallback_t setSPS30AutoCleanIntervalCallback,
    cleanSPS30Callback_t cleanSPS30Callback,
    getSPS30StatusCallback_t getSPS30StatusCallback
  );

  void publishSensors(uint16_t mask);
  void publishConfiguration();
  void publishStatusMsg(const char* statusMessage);

  void mqttLoop(void* pvParameters);

  extern TaskHandle_t mqttTask;
}

#endif