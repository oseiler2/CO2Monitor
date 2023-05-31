#ifndef _NEOPIXEL_H
#define _NEOPIXEL_H

#include <globals.h>
#include <Arduino.h>
#include <model.h>
#include <Adafruit_NeoPixel.h>

#include <Ticker.h>

class Neopixel {
public:
  Neopixel(Model* model, uint8_t pin, uint8_t numPixel);
  ~Neopixel();

  void update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus);
  void off();

private:
  void fill(uint32_t c);
  void timer();

  Adafruit_NeoPixel* strip;
  Model* model;
  Ticker* cyclicTimer;
  boolean toggle;

  uint32_t colourRed;
  uint32_t colourYellow;
  uint32_t colourGreen;
  uint32_t colourPurple;
  uint32_t colourOff;
};

#endif