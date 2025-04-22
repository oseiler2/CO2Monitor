#pragma once

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
  void setLargePriorityMessage(char const* msg);
  void clearLargePriorityMessage();

  void showMenu(char const* heading, char const* selection);
  void quitMenu();

private:
  Model* model;

  Adafruit_SSD1306* display;

  boolean priorityMessageActive;
  boolean largePriorityMessageActive;
  boolean menuActive;

  uint8_t status_y;
  uint8_t status_height;
  uint8_t temp_hum_y;
  uint8_t temp_hum_height;
  uint8_t line1_y;
  uint8_t line2_y;
  uint8_t line3_y;
  uint8_t line_height;

};

