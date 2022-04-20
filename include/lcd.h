#ifndef _LCD_H
#define _LCD_H

#include <globals.h>
#include <Wire.h>
#include <model.h>

#include <Adafruit_SSD1306.h>

class LCD {
public:
  LCD(TwoWire* pwire, Model* model);
  ~LCD();

  void updateMessage(char const* msg);
  void update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus);

private:
  Model* model;

  Adafruit_SSD1306* display;
};


#endif