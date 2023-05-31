#ifndef _SPS30_H
#define _SPS30_H

#include <Arduino.h>
#include <globals.h>
#include <config.h>
#include <messageSupport.h>
#include <Wire.h>
#include <model.h>
#include <sps30.h>

class SPS_30 {
public:
  SPS_30(TwoWire* pwire, Model* _model, updateMessageCallback_t _updateMessageCallback);
  ~SPS_30();

  boolean readSps30();
  uint32_t getInterval();

  uint32_t getAutoCleanInterval();
  boolean setAutoCleanInterval(uint32_t intervalInSeconds);
  boolean clean();
  uint8_t getStatus();

private:
  Model* model;
  SPS30* sps30;
  updateMessageCallback_t updateMessageCallback;

  boolean checkError(uint16_t error, char const* msg);
  static void sps30Loop(void* pvParameters);
};

#endif