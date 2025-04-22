#pragma once

#include <globals.h>
#include <config.h>
#include <model.h>
#include <Ticker.h>

#if defined (HAS_HUB75)

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

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
