#pragma once

#include <globals.h>
#include <config.h>
#include <callbacks.h>
#include <Wire.h>
#include <model.h>
#include <SensirionI2CScd4x.h>

typedef enum {
  PERIODIC = 0,
  LP_PERIODIC
} SCD40SampleRate;

class SCD40 {
public:
  SCD40(TwoWire* pwire, Model* _model, updateMessageCallback_t _updateMessageCallback, boolean initFromSleep);
  ~SCD40();

  boolean readScd40();
  uint32_t getInterval();

  boolean calibrateScd40ToReference(uint16_t co2Reference);
  boolean setTemperatureOffset(float temperatureOffset);
  float getTemperatureOffset();
  boolean setAmbientPressure(uint16_t ambientPressureInHpa);

  boolean setSampleRate(SCD40SampleRate sampleRate);
  void shutdown();

private:
  Model* model;
  SensirionI2CScd4x* scd40;
  updateMessageCallback_t updateMessageCallback;
  uint16_t lastAmbientPressure = 0x0000;

  boolean startMeasurement();

  boolean checkError(uint16_t error, char const* msg);
  static void scd40Loop(void* pvParameters);

  SCD40SampleRate sampleRate;
};
