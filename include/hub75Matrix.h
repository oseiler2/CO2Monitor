#ifndef _HUB75_MATRIX_H
#define _HUB75_MATRIX_H

#include <Arduino.h>
#include <globals.h>
#include <config.h>
#include <model.h>
#include <Ticker.h>

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

class HUB75Matrix {
public:
  HUB75Matrix(Model* _model, uint8_t columns, uint8_t rows);
  ~HUB75Matrix();

  void update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus);
  TaskHandle_t start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core);
  void stop();

  void stopDMA();

private:
  static void hub75MatrixLoop(void* pvParameters);
  uint16_t ppmToDots(uint16_t ppm);
  void showTank(uint16_t ppm, bool showDrip);
  void showText();
  void waveTimer();
  void dripTimer();
  void snakeTimer();
  void textTimer();
  void update(uint16_t ppm);
  uint16_t ppmToColour(uint16_t ppm);

  Model* model;

  MatrixPanel_I2S_DMA* matrix;
  Ticker* cyclicTimer;
  Ticker* snakeTicker;

  typedef enum {
    TIMER_OFF = 0,
    TIMER_WAVE,
    TIMER_DRIP,
    TIMER_SCROLL
  } CyclicTimerMode;

  CyclicTimerMode cyclicTimerMode = TIMER_OFF;

  TaskHandle_t hub75MatrixTask;

  typedef enum {
    SHOW_TANK = 0,
    SHOW_TEXT
  } TankPpmMode;

  TankPpmMode displayMode = SHOW_TANK;
  bool rotateTank = false;

  QueueHandle_t updateQueue;
  TaskHandle_t displayTask;

  uint16_t GREEN;
  uint16_t YELLOW;
  uint16_t RED;

  struct QueueMessage {
    uint8_t cmd;
    uint16_t ppm;
  };

  const static uint8_t X_CMD_SHOW_PPM = bit(0);
  const static uint8_t X_CMD_SHOW_DRIP = bit(1);
  const static uint8_t X_CMD_SHOW_TEXT = bit(2);

  uint8_t HUB75_MATRIX_WIDTH;
  uint8_t HUB75_MATRIX_HEIGHT;
  uint16_t NUMBER_OF_DOTS;

  uint8_t TANK_WIDTH;
  uint8_t TANK_HEIGHT;

  uint16_t LOWER_LIMIT;
  uint16_t MID_POINT;
  uint16_t UPPER_LIMIT;
  uint16_t RANGE;
  float PPM_PER_DOT;

  uint8_t textSize = 1;

  volatile uint16_t currentPpm = 0, previousPpm = 0;
  volatile uint32_t lastPpmUpdate;

  // === DRIP ===
  volatile uint8_t dripColumn;
  volatile uint8_t dripCurrentRow;
  volatile uint8_t dripFinalRow;
  volatile int8_t dripDirection;
  const static uint16_t DRIP_TIMER_INTERVAL = 20;

  // === WAVE ===
  const float dampen = 0.07;
  volatile float amplitude;
  const static uint16_t WAVE_TIMER_INTERVAL = 50;

  // === SNAKES ===
  const uint8_t NUMBER_OF_SNAKES = 6;
  const uint8_t SNAKE_LENGTH = 32;
  volatile uint16_t snakePos = 0;
  const static uint16_t SNAKE_TICKER_INTERVAL = 100;

  // === PPM DISPLAY ===
  char txt[6];
  int16_t scrollWidth;
  volatile int16_t scrollPosition;
  volatile int8_t scrollDirection;
  const static uint16_t TEXT_TIMER_INTERVAL = 500;

};

#endif
