#ifndef _TRAFFIC_LIGHT_H
#define _TRAFFIC_LIGHT_H

#include <globals.h>
#include <Arduino.h>
#include <model.h>
#include <Adafruit_NeoPixel.h>

#include <Ticker.h>

class Neopixel {
public:
  Neopixel(Model* model, uint8_t pin, uint8_t numPixel);
  ~Neopixel();

  void update();

private:
  void fill(uint32_t c);
  void timer();

  typedef enum { UNDEFINED, GREEN, YELLOW, RED, DARK_RED } NeopixelStatus;

  Adafruit_NeoPixel* strip;
  Model* model;
  NeopixelStatus status;
  Ticker* cyclicTimer;
  boolean toggle;
};

#endif