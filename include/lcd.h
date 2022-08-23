#ifndef _LCD_H
#define _LCD_H

#include <globals.h>
#include <Wire.h>
#include <model.h>

#include <Adafruit_SSD1306.h>

class LCD {
public:
  LCD(TwoWire* pwire, Model* model, boolean initFromSleep);
  ~LCD();

  void updateMessage(char const* msg);
  void update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus);
  void setPriorityMessage(char const* msg);
  void clearPriorityMessage();

  void showMenu(char const* heading, char const* selection);
  void quitMenu();

private:
  Model* model;

  Adafruit_SSD1306* display;

  boolean priorityMessageActive;
  boolean menuActive;
};


#endif