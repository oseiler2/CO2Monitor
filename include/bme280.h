#pragma once

#include <globals.h>
#include <config.h>
#include <callbacks.h>
#include <Wire.h>
#include <model.h>
#include <Adafruit_BME280.h>

class BME280 {
public:
  BME280(TwoWire* pwire, Model* _model, updateMessageCallback_t _updateMessageCallback, boolean initFromSleep);
  ~BME280();

  boolean readBme280();
  uint32_t getInterval();

  boolean setSampleRate(float sampleRate);
  void shutdown();

private:
  Adafruit_BME280* bme280;
  Model* model;
  updateMessageCallback_t updateMessageCallback;

  float sampleRate;

  static void bme280Loop(void* pvParameters);

};
