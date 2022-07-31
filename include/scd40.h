#ifndef _SCD40_H
#define _SCD40_H

#include <Arduino.h>
#include "globals.h"
#include <config.h>
#include <messageSupport.h>
#include <Wire.h>
#include <model.h>
#include <SensirionI2CScd4x.h>

class SCD40 {
public:
  SCD40(TwoWire* pwire, Model* _model, updateMessageCallback_t _updateMessageCallback);
  ~SCD40();

  boolean readScd40();
  uint32_t getInterval();

  boolean calibrateScd40ToReference(uint16_t co2Reference);
  boolean setTemperatureOffset(float temperatureOffset);
  float getTemperatureOffset();
  boolean setAmbientPressure(uint16_t ambientPressureInHpa);

private:
  Model* model;
  SensirionI2CScd4x* scd40;
  updateMessageCallback_t updateMessageCallback;
  uint16_t lastAmbientPressure = 0x0000;

  boolean checkError(uint16_t error, char const* msg);
  static void scd40Loop(void* pvParameters);
};

#endif