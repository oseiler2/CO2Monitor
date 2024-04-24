#ifndef _BME680_H
#define _BME680_H

#include <Arduino.h>
#include "globals.h"
#include <config.h>
#include <callbacks.h>
#include <Wire.h>
#include <model.h>
#include <EEPROM.h>
#include "bsec.h"

typedef enum {
  ULP = 0,
  LP,
  CONTINUOUS
} Bme680SampleRate;

class BME680 {
public:
  BME680(TwoWire* pwire, Model* _model, updateMessageCallback_t _updateMessageCallback, boolean initFromSleep);
  ~BME680();

  boolean readBme680();
  uint32_t getInterval();

  boolean setSampleRate(Bme680SampleRate sampleRate);
  void shutdown();

private:
  Bsec* bme680;
  Model* model;
  updateMessageCallback_t updateMessageCallback;

  float sampleRate;

  static void bme680Loop(void* pvParameters);

  void checkIaqSensorStatus();
  void loadState(void);
  void updateState(void);

};

#endif