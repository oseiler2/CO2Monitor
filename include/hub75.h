#if CONFIG_IDF_TARGET_ESP32

#ifndef _HUB75_H
#define _HUB75_H

#include <Arduino.h>
#include "globals.h"
#include <config.h>
#include <model.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include <Ticker.h>

class HUB75 {
public:
  HUB75(Model* _model);
  ~HUB75();

  void update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus);
  void stopDMA();

private:
  Model* model;
  MatrixPanel_I2S_DMA* matrix;

  void timer();
  Ticker* cyclicTimer;
  boolean toggle;
};

#endif

#endif