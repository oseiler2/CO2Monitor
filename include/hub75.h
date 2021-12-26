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

  void update();

private:
  Model* model;
  MatrixPanel_I2S_DMA* matrix;

  typedef enum { UNDEFINED = -1, GREEN = 0, YELLOW = 1, RED = 2, DARK_RED = 3 } Status;

  Status status;

  void timer();
  Ticker* cyclicTimer;
  boolean toggle;
};

#endif