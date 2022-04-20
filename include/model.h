#ifndef _MODEL_H
#define _MODEL_H

#include <Arduino.h>

const float NaN = sqrt(-1);

typedef enum {
  M_NONE = 0,
  M_CO2 = 1 << 0,
  M_TEMPERATURE = 1 << 1,
  M_HUMIDITY = 1 << 2,
  M_PRESSURE = 1 << 3,
  M_IAQ = 1 << 4
} Measurement;

typedef enum {
  UNDEFINED = 0,
  GREEN,
  YELLOW,
  RED,
  DARK_RED
} TrafficLightStatus;

typedef void (*modelUpdatedEvt_t)(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus);

class Model {
public:

  Model(modelUpdatedEvt_t modelUpdatedEvt);
  ~Model();

  uint16_t getCo2();
  float getTemperature();
  float getHumidity();
  uint16_t getPressure();
  uint16_t getIAQ();

  TrafficLightStatus getStatus();

  void updateModel(uint16_t co2, float temperature, float humidity);
  void updateModel(float temperature, float humidity, uint16_t pressure, uint16_t iaq);

private:

  TrafficLightStatus status;
  float temperature;
  float humidity;
  uint16_t co2;
  uint16_t pressure;
  uint16_t iaq;
  modelUpdatedEvt_t modelUpdatedEvt;
  void updateStatus();

};

#endif