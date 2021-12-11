#ifndef _BME680_H
#define _BME680_H

#include <Arduino.h>
#include "globals.h"
#include <config.h>
#include <Wire.h>
#include <model.h>
#include <EEPROM.h>
#include "bsec.h"

class BME680 {
public:
  BME680(TwoWire* pwire, Model* _model, updateMessageCallback_t _updateMessageCallback);
  ~BME680();

  TaskHandle_t start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core);

private:
  Bsec* bme680;
  Model* model;
  updateMessageCallback_t updateMessageCallback;

  TaskHandle_t task;

  boolean readBme680();
  static void bme680Loop(void* pvParameters);


  void checkIaqSensorStatus();
};

#endif