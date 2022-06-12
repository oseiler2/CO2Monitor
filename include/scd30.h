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

  TaskHandle_t start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core);
  boolean calibrateScd30ToReference(uint16_t co2Reference);
  boolean setTemperatureOffset(float temperatureOffset);
  float getTemperatureOffset();

private:
  Model* model;
  Adafruit_SCD30* scd30;
  updateMessageCallback_t updateMessageCallback;

  TaskHandle_t task;

  boolean readScd30();

  static void scd30Loop(void* pvParameters);
};

#endif