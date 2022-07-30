#ifndef _TRAFFIC_LIGHT_H
#define _TRAFFIC_LIGHT_H

#include <globals.h>
#include <Arduino.h>
#include <config.h>
#include <model.h>

#include <Ticker.h>

class TrafficLight {
public:
  TrafficLight(Model* model, uint8_t pinRed, uint8_t pinYellow, uint8_t pinGreen);
  ~TrafficLight();

  void update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus);

private:
  void timer();

  Model* model;
  uint8_t pinRed;
  uint8_t pinYellow;
  uint8_t pinGreen;
  Ticker* cyclicTimer;
};

#endif