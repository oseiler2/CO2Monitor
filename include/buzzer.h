#pragma once

#include <globals.h>
#include <config.h>
#include <model.h>

#include <Ticker.h>

class Buzzer {
public:
  Buzzer(Model* model, uint8_t buzzerPin, boolean initFromSleep);
  ~Buzzer();

  void update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus);
  void beep(uint8_t n);
  void alert();

private:

  const uint8_t DARK_RED_BUZZES = 3;

  volatile uint8_t buzzCtr = 0;


  void timer();

  Model* model;
  uint8_t buzzerPin;
  Ticker* cyclicTimer;
};
