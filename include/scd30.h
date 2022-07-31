#ifndef _SCD30_H
#define _SCD30_H

#include <Arduino.h>
#include "globals.h"
#include <config.h>
#include <messageSupport.h>
#include <Wire.h>
#include <model.h>
#include <Adafruit_SCD30.h>

class SCD30 {
public:
  SCD30(TwoWire* pwire, Model* _model, updateMessageCallback_t _updateMessageCallback);
  ~SCD30();

  boolean readScd30();
  uint32_t getInterval();

  boolean calibrateScd30ToReference(uint16_t co2Reference);
  boolean setTemperatureOffset(float temperatureOffset);
  float getTemperatureOffset();
  boolean setAmbientPressure(uint16_t ambientPressureInHpa);

private:
  Model* model;
  Adafruit_SCD30* scd30;
  updateMessageCallback_t updateMessageCallback;
  boolean initialised = false;
  uint16_t lastAmbientPressure = 0x0000;

  static void scd30Loop(void* pvParameters);
};

#endif