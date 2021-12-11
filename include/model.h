#ifndef _MODEL_H
#define _MODEL_H

#include <Arduino.h>

const float NaN = sqrt(-1);

typedef void (*modelUpdatedEvt_t)(void);

class Model {
public:

  Model(modelUpdatedEvt_t modelUpdatedEvt);
  ~Model();

  uint16_t getCo2();
  float getTemperature();
  float getHumidity();

  void updateModel(uint16_t co2, float temperature, float humidity);

private:
  float temperature;
  float humidity;
  uint16_t co2;

  modelUpdatedEvt_t modelUpdatedEvt;

};

#endif