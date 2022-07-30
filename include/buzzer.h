#ifndef _BUZZER_H
#define _BUZZER_H

#include <globals.h>
#include <Arduino.h>
#include <config.h>
#include <model.h>

#include <Ticker.h>

class Buzzer {
public:
  Buzzer(Model* model, uint8_t buzzerPin);
  ~Buzzer();

  void update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus);

private:

  const uint8_t DARK_RED_BUZZES = 3;

  volatile uint8_t buzzCtr = 0;


  void timer();

  Model* model;
  uint8_t buzzerPin;
  Ticker* cyclicTimer;
};

#endif