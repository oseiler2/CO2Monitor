#ifndef _MODEL_H
#define _MODEL_H

#include <Arduino.h>

const float NaN = sqrt(-1);

typedef void (*modelUpdatedEvt_t)(uint16_t mask);

typedef enum {
  M_NONE = 0,
  M_CO2 = 1 << 0,
  M_TEMPERATURE = 1 << 1,
  M_HUMIDITY = 1 << 2,
  M_PRESSURE = 1 << 3,
  M_IAQ = 1 << 4
} Measurement;

class Model {
public:

  Model(modelUpdatedEvt_t modelUpdatedEvt);
  ~Model();

  uint16_t getCo2();
  float getTemperature();
  float getHumidity();
  uint16_t getPressure();
  uint16_t getIAQ();

  void updateModel(uint16_t co2, float temperature, float humidity);
  void updateModel(float temperature, float humidity, uint16_t pressure, uint16_t iaq);

private:
  float temperature;
  float humidity;
  uint16_t co2;
  uint16_t pressure;
  uint16_t iaq;
  modelUpdatedEvt_t modelUpdatedEvt;

};

#endif