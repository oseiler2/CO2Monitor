#ifndef _FEATHER_MATRIX_H
#define _FEATHER_MATRIX_H

#include <globals.h>
#include <Arduino.h>
#include <model.h>
#include <Adafruit_DotStarMatrix.h>

#include <Ticker.h>

class FeatherMatrix {
public:
  FeatherMatrix(Model* model, uint8_t dataPin, uint8_t clockPin);
  ~FeatherMatrix();

  void update(uint16_t, TrafficLightStatus oldStatus, TrafficLightStatus newStatus);

private:
  void timer();

  Model* model;

  Adafruit_DotStarMatrix* matrix;
  Ticker* cyclicTimer;

  char txt[6];
  int16_t scrollPosition;
  uint16_t textWidth;
  int8_t scrollDirection;
  int16_t scrollWidth;
};

#endif

