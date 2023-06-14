#ifndef _MQTT_H
#define _MQTT_H

#include <globals.h>
#include <ArduinoJson.h>
#include <messageSupport.h>

// If you issue really large certs (e.g. long CN, extra options) this value may need to be
// increased, but 1600 is plenty for a typical CN and standard option openSSL issued cert.
#define MQTT_CERT_SIZE 8192

// Use larger of cert or config for MQTT buffer size.
#define MQTT_BUFFER_SIZE MQTT_CERT_SIZE > CONFIG_SIZE ? MQTT_CERT_SIZE : CONFIG_SIZE


namespace mqtt {
  typedef void (*calibrateCo2SensorCallback_t)(uint16_t);
  typedef void (*setTemperatureOffsetCallback_t)(float);
  typedef float (*getTemperatureOffsetCallback_t)(void);
  typedef uint32_t(*getSPS30AutoCleanIntervalCallback_t)(void);
  typedef boolean(*setSPS30AutoCleanIntervalCallback_t)(uint32_t);
  typedef boolean(*cleanSPS30Callback_t)(void);
  typedef uint8_t(*getSPS30StatusCallback_t)(void);

  void setupMqtt(
    calibrateCo2SensorCallback_t calibrateCo2SensorCallback,
    setTemperatureOffsetCallback_t setTemperatureOffsetCallback,
    getTemperatureOffsetCallback_t getTemperatureOffsetCallback,
    getSPS30AutoCleanIntervalCallback_t getSPS30AutoCleanIntervalCallback,
    setSPS30AutoCleanIntervalCallback_t setSPS30AutoCleanIntervalCallback,
    cleanSPS30Callback_t cleanSPS30Callback,
    getSPS30StatusCallback_t getSPS30StatusCallback,
    configChangedCallback_t configChangedCallback
  );

  void publishSensors(DynamicJsonDocument* _payload);
  void publishConfiguration();
  void publishStatusMsg(const char* statusMessage);

  void mqttLoop(void* pvParameters);

  extern TaskHandle_t mqttTask;
}

#endif