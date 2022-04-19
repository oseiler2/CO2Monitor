#ifndef _NEOPIXEL_H
#define _NEOPIXEL_H

#include <globals.h>
#include <Arduino.h>
#include <model.h>

#include <Ticker.h>

class TrafficLight {
public:
  TrafficLight(Model* model, uint8_t pinRed, uint8_t pinYellow, uint8_t pinGreen);
  ~TrafficLight();

  void update(uint16_t mask);

private:
  void timer();

  typedef enum { UNDEFINED, GREEN, YELLOW, RED, DARK_RED } TrafficLightStatus;

  Model* model;
  TrafficLightStatus status;
  uint8_t pinRed;
  uint8_t pinYellow;
  uint8_t pinGreen;
  Ticker* cyclicTimer;
};

#endif