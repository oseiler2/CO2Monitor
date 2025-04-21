#pragma once

#include <globals.h>

const float NaN = sqrt(-1);

typedef enum {
  M_NONE = 0,
  M_CO2 = 1 << 0,
  M_TEMPERATURE = 1 << 1,
  M_HUMIDITY = 1 << 2,
  M_PRESSURE = 1 << 3,
  M_IAQ = 1 << 4,
  M_PM0_5 = 1 << 5,
  M_PM1_0 = 1 << 6,
  M_PM2_5 = 1 << 7,
  M_PM4 = 1 << 8,
  M_PM10 = 1 << 9,
  M_VOLTAGE = 1 << 10,
  M_RUN_MODE = 1 << 11,
  M_CONFIG_CHANGED = 1 << 12
} Measurement;

typedef enum {
  OFF = 0,
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
  uint16_t getPM0_5();
  uint16_t getPM1();
  uint16_t getPM2_5();
  uint16_t getPM4();
  uint16_t getPM10();
  uint16_t getVoltageInMv();

  TrafficLightStatus getStatus();
  void setStatus(TrafficLightStatus status);

  void updateModel(uint16_t _co2);
  void updateModel(uint16_t co2, float temperature, float humidity);
  void updateModel(float temperature, float humidity, uint16_t pressure, uint16_t iaq);
  void updateModel(uint16_t pm0_5, uint16_t pm1, uint16_t pm2_5, uint16_t pm4, uint16_t pm10);
  void updateModelmV(uint16_t mV);
  void runModeChanged();
  void configurationChanged();

private:

  TrafficLightStatus status;
  float temperature;
  float humidity;
  uint16_t co2;
  uint16_t pressure;
  uint16_t iaq;
  uint16_t pm0_5;
  uint16_t pm1;
  uint16_t pm2_5;
  uint16_t pm4;
  uint16_t pm10;
  uint16_t voltageInMv;
  modelUpdatedEvt_t modelUpdatedEvt;
  void updateStatus();

};
