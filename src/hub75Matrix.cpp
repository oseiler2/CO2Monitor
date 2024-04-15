#include <hub75Matrix.h>
#include <Fonts/TomThumb.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <config.h>
#include <configManager.h>

// Local logging tag
static const char TAG[] = __FILE__;

#define USE_FASTLINES

#define ENABLE_TEXT   false
#define TANK_INTERVAL 30000
#define TEXT_INTERVAL 5000

HUB75Matrix::HUB75Matrix(Model* _model, uint8_t _columns, uint8_t _rows) {
  this->model = _model;
  HUB75_MATRIX_WIDTH = _columns;
  HUB75_MATRIX_HEIGHT = _rows;
  NUMBER_OF_DOTS = HUB75_MATRIX_WIDTH * HUB75_MATRIX_HEIGHT;
  TANK_WIDTH = rotateTank ? HUB75_MATRIX_HEIGHT : HUB75_MATRIX_WIDTH;
  TANK_HEIGHT = rotateTank ? HUB75_MATRIX_WIDTH : HUB75_MATRIX_HEIGHT;

  LOWER_LIMIT = 450;
  UPPER_LIMIT = config.co2DarkRedThreshold;
  MID_POINT = config.co2YellowThreshold;
  RANGE = UPPER_LIMIT - LOWER_LIMIT;
  PPM_PER_DOT = (float)RANGE / (NUMBER_OF_DOTS);

  ESP_LOGD(TAG, "HUB75Matrix initialised");
}

HUB75Matrix::~HUB75Matrix() {
  if (this->matrix) delete matrix;
  if (this->cyclicTimer) delete cyclicTimer;
  if (this->snakeTicker) delete snakeTicker;
}

TaskHandle_t HUB75Matrix::start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core) {
  if (xPortGetCoreID() != 1)
    ESP_LOGW(TAG, ">>>>>>>>>>>>>>> Running on core %u instead of core 1", xPortGetCoreID());
  xTaskCreatePinnedToCore(
    this->hub75MatrixLoop,  // task function
    name,                      // name of task
    stackSize,                 // stack size of task
    this,                      // parameter of the task
    priority,                  // priority of the task
    &displayTask,              // task handle
    core);                     // CPU core
  return displayTask;
}

void HUB75Matrix::stop() {
  vTaskSuspend(displayTask);
  updateQueue = NULL;
  if (snakeTicker->active()) snakeTicker->detach();
  if (cyclicTimer->active()) cyclicTimer->detach();
  cyclicTimerMode = TIMER_OFF;
  if (matrix) {
    matrix->setBrightness(0);
    matrix->fillScreen(0);
    matrix->flipDMABuffer();
    matrix->fillScreen(0);
  }
}

void HUB75Matrix::stopDMA() {
  if (this->matrix) matrix->stopDMAoutput();
}

/**
 * Return the number of dots that represent the given PPM value, with ppm <= LOWER_LIMIT returning NUMBER_OF_DOTS (= full tank) and
 * ppm >= UPPER_LIMIT returning 0 (= empty tank)
 */
uint16_t HUB75Matrix::ppmToDots(uint16_t ppm) {
  ppm = max(min(ppm, UPPER_LIMIT), LOWER_LIMIT);
  return NUMBER_OF_DOTS - min((uint16_t)floor((ppm - LOWER_LIMIT) / PPM_PER_DOT), NUMBER_OF_DOTS);
}

/**
 * Returns a colour for the given PPM value, with
 * ppm == LOWER_LIMIT returning GREEN,
 * ppm == mid point (LOWER_LIMIT, UPPER_LIMIT) returning YELLOW and
 * ppm == UPPER_LIMIT returning RED.
 * Values in between are interpolated.
 */
uint16_t HUB75Matrix::ppmToColour(uint16_t ppm) {
  if (ppm < (MID_POINT)) {
    // green - yellow
    float d = float(max(ppm, LOWER_LIMIT) - LOWER_LIMIT) / float(MID_POINT - LOWER_LIMIT);
    return matrix->color565((uint8_t)(255.0 * d), 255, 0);
  } else {
    // yellow - red
    float d = float(min(ppm, config.co2RedThreshold) - (MID_POINT)) / float(config.co2RedThreshold - MID_POINT);
    return  matrix->color565(255, (uint8_t)(255.0 * (1.0 - d)), 0);
  }
}

void HUB75Matrix::showText() {
  if (this->displayMode != SHOW_TEXT) return;
  matrix->setBrightness(config.brightness);
  matrix->fillScreen(0);
  matrix->setTextColor(matrix->color565(255, 255, 255));//ppmToColour(currentPpm));
  matrix->setCursor(1 + scrollPosition + (scrollWidth > 0 ? 1 : 0), HUB75_MATRIX_HEIGHT / 2 - (16 * textSize / 2));
  matrix->print(txt);
  matrix->flipDMABuffer();
}

/**
 * Render the given PPM value on the matrix
 */
void HUB75Matrix::showTank(uint16_t ppm, bool showDrip) {
  if (this->displayMode != SHOW_TANK) return;
  uint16_t color = ppmToColour(ppm); //showDrip ? GREEN : GREEN; //ppmToColour(ppm);

  // takes about  1297 micros
  //  uint32_t start = micros();
  uint16_t dots = ppmToDots(ppm);

  float eps = 1 - ((float)dots / NUMBER_OF_DOTS);
  matrix->setBrightness(min(int(config.brightness + (eps * 10)), 255));

  matrix->fillScreen(0);
  //  matrix->fillScreen(matrix->color565(255, 0, 255));
  uint8_t rows = min((uint8_t)floor(dots / TANK_WIDTH), TANK_HEIGHT);                   // number of full rows
  uint8_t remainder = dots % TANK_WIDTH;                                                  // number of additional dots
  //  ESP_LOGI(TAG, "show() ppm %u, dots %u, colums %u, remainder %u", ppm, dots, rows, remainder);

  if (rows > 0) matrix->fillRect(0, TANK_HEIGHT - rows, TANK_WIDTH, rows, color);        // render full rows
  if (remainder == 1) matrix->drawPixel(0, TANK_HEIGHT - rows - 1, color);                 // render additional dots (== 1)
  if (remainder > 1) matrix->drawFastHLine(0, TANK_HEIGHT - rows - 1, remainder, color);   // render additional dots (> 1)
  if (showDrip) matrix->drawPixel(dripColumn, TANK_HEIGHT - 1 - dripCurrentRow, dripDirection > 0 ? GREEN : color);

  // if snake(s) are active, draw snake(s) at current positions - dows not move the snake
  if (snakeTicker->active()) {
    //    matrix->setTextColor(matrix->color565(255, 255, 255));//ppmToColour(currentPpm));
    matrix->setCursor(scrollPosition + (scrollWidth > 0 ? 1 : 0), HUB75_MATRIX_HEIGHT / 2 + (8 * textSize / 2));
    matrix->print(txt);
    for (uint8_t s = 0; s < NUMBER_OF_SNAKES; s++) {
      uint16_t snakeOffset = s * (((TANK_WIDTH + TANK_HEIGHT - 2) * 2)) / NUMBER_OF_SNAKES;
      for (uint8_t i = 0; i < SNAKE_LENGTH; i++) {
        uint16_t p = (snakePos + i + snakeOffset) % ((TANK_WIDTH + TANK_HEIGHT - 2) * 2);
        if (p < TANK_WIDTH) {
          // ESP_LOGD(TAG, "snakePos 1 %u =>(%u,%u)", p, p, TANK_HEIGHT - 1);
          matrix->drawPixel(p, TANK_HEIGHT - 1, RED);
        } else if (p < TANK_WIDTH + TANK_HEIGHT - 1) {
          p -= TANK_WIDTH;
          // ESP_LOGD(TAG, "snakePos 2 %u =>(%u,%u)", p, TANK_WIDTH - 1, TANK_HEIGHT - 1 - p - 1);
          matrix->drawPixel(TANK_WIDTH - 1, TANK_HEIGHT - 1 - p - 1, RED);
        } else if (p < (2 * TANK_WIDTH) + TANK_HEIGHT - 2) {
          p -= (TANK_WIDTH + TANK_HEIGHT - 1);
          // ESP_LOGD(TAG, "snakePos 3 %u =>(%u,%u)", p, TANK_WIDTH - p - 2, 0);
          matrix->drawPixel(TANK_WIDTH - p - 2, 0, RED);
        } else {
          p -= (2 * TANK_WIDTH + TANK_HEIGHT - 2);
          // ESP_LOGD(TAG, "snakePos 4 %u =>(%u,%u)", p, 0, p + 1);
          matrix->drawPixel(0, p + 1, RED);
        }
      }
    }
  }

  //  uint32_t end = micros();
  //  ESP_LOGI(TAG, "show() took %u us", end - start);
  matrix->flipDMABuffer();
}

void HUB75Matrix::update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
  if (mask & M_CONFIG_CHANGED) {
    UPPER_LIMIT = config.co2DarkRedThreshold;
    MID_POINT = config.co2YellowThreshold;
    RANGE = UPPER_LIMIT - LOWER_LIMIT;
    PPM_PER_DOT = (float)RANGE / (NUMBER_OF_DOTS);
    this->update(model->getCo2());
    matrix->setBrightness(config.brightness);
  }
  if (mask & M_CO2) this->update(model->getCo2());
}

/**
 * Provide a new, updated ppm reading. If the new reading differs 2 or more dots from previous reading the wave animation
 * will be started. Otherwise a drop will 'drip' in or out.
 */
void HUB75Matrix::update(uint16_t ppm) {
  // ESP_LOGD(TAG, "update: %u", ppm);
  previousPpm = currentPpm;
  currentPpm = ppm;

  if (ppm == 0) {
    strcpy(txt, "---");
  } else {
    sprintf(txt, "%u", ppm);
  }

  if (displayMode == SHOW_TEXT) {
    if (snakeTicker->active()) snakeTicker->detach();
    if (cyclicTimerMode != TIMER_SCROLL && cyclicTimer->active()) {
      cyclicTimer->detach();
      cyclicTimerMode = TIMER_OFF;
    }
    scrollPosition = 0;
    int16_t x1, y1 = 0;
    uint16_t textWidth, h = 0;
    matrix->getTextBounds(txt, 0, 0, &x1, &y1, &textWidth, &h);
    scrollWidth = textWidth - matrix->width();
    if (scrollWidth > 0) {
      scrollWidth += 2;
    }
    // ESP_LOGD(TAG, "textTimer=> scrollPosition %i, textWidth %u, scrollWidth %i, scrollDirection %i", scrollPosition, textWidth, scrollWidth, scrollDirection);
    if (scrollWidth > 0) {
      if (!cyclicTimer->active()) {
        cyclicTimerMode = TIMER_SCROLL;
        cyclicTimer->attach_ms(TEXT_TIMER_INTERVAL, +[](HUB75Matrix* instance) { instance->textTimer(); }, this);
      }
    } else {
      if (cyclicTimer->active()) cyclicTimer->detach();
      cyclicTimerMode = TIMER_OFF;
      QueueMessage msg;
      msg.cmd = X_CMD_SHOW_TEXT;
      msg.ppm = currentPpm;
      if (updateQueue) xQueueSendToBack(updateQueue, (void*)&msg, pdMS_TO_TICKS(5));
    }
  } else if (displayMode == SHOW_TANK) {
    uint16_t delta = (uint16_t)abs(previousPpm - currentPpm);
    if (ppm <= UPPER_LIMIT && delta > 0 && delta < 20 * PPM_PER_DOT) {
      if (previousPpm > currentPpm) {
        // ppm decreasing
        dripDirection = 1;
        uint16_t dots = ppmToDots(ppm);
        dripCurrentRow = TANK_HEIGHT;
        dripColumn = max(dots - 1, 0) % TANK_WIDTH;
        dripFinalRow = min((uint8_t)floor((dots - 1) / TANK_WIDTH), TANK_HEIGHT);
        //      ESP_LOGD(TAG, "dripTimer=> dripColumn %u, dripFinalRow %u", dripColumn, dripFinalRow);
      } else {
        // ppm increasing
        dripDirection = -1;
        uint16_t dots = ppmToDots(previousPpm);
        dripCurrentRow = min((uint8_t)floor((dots - 1) / TANK_WIDTH), TANK_HEIGHT);
        dripColumn = max(dots - 1, 0) % TANK_WIDTH;
        dripFinalRow = TANK_HEIGHT - 1;
        //      ESP_LOGD(TAG, "dripTimer=> dripColumn %u, dripCurrentRow %u", dripColumn, dripCurrentRow);
      }
      if (cyclicTimer->active()) cyclicTimer->detach();
      cyclicTimerMode = TIMER_DRIP;
      cyclicTimer->attach_ms(DRIP_TIMER_INTERVAL, +[](HUB75Matrix* instance) { instance->dripTimer(); }, this);
    } else if (ppm <= UPPER_LIMIT && delta >= 2 * PPM_PER_DOT) {
      amplitude = 1;
      lastPpmUpdate = millis();
      if (cyclicTimer->active()) cyclicTimer->detach(); // cyclicTimer could be active on DRIP, hence detach
      cyclicTimerMode = TIMER_WAVE;
      cyclicTimer->attach_ms(WAVE_TIMER_INTERVAL, +[](HUB75Matrix* instance) { instance->waveTimer(); }, this);
    } else {
      QueueMessage msg;
      msg.cmd = X_CMD_SHOW_PPM;
      msg.ppm = ppm;
      if (updateQueue) xQueueSendToBack(updateQueue, (void*)&msg, pdMS_TO_TICKS(5));
    }
    // turn on snake(s) if only 1 row or less is visible
    if (currentPpm > UPPER_LIMIT - (TANK_WIDTH * PPM_PER_DOT)) {
      if (!snakeTicker->active()) snakeTicker->attach_ms(SNAKE_TICKER_INTERVAL, +[](HUB75Matrix* instance) { instance->snakeTimer(); }, this);
    } else {
      if (snakeTicker->active()) snakeTicker->detach();
    }
  }
}

void HUB75Matrix::waveTimer() {
  if (amplitude < 0.05f || abs(currentPpm - previousPpm) * (amplitude) < PPM_PER_DOT) {
    cyclicTimer->detach();
    cyclicTimerMode = TIMER_OFF;
    if (currentPpm > UPPER_LIMIT - (TANK_WIDTH * PPM_PER_DOT)) {
      // turn on snake(s) if only 1 row or less is visible
      if (!snakeTicker->active()) snakeTicker->attach_ms(SNAKE_TICKER_INTERVAL, +[](HUB75Matrix* instance) { instance->snakeTimer(); }, this);
      return;
    }
  }
  uint16_t timePassed = millis() - lastPpmUpdate;
  float theta = float(timePassed) / 100; // 300
  uint16_t tempPpm = currentPpm - cos(theta) * (currentPpm - previousPpm) * (amplitude);
  // ESP_LOGD(TAG, "timer() timePassed %u, theta %.1f, cos %.1f, amplitude %.2f, delta %d %.1f, tempPpm %u", timePassed, theta, cos(theta), amplitude, (currentPpm - previousPpm), (currentPpm - previousPpm) / PPM_PER_DOT, tempPpm);
  amplitude -= amplitude * dampen;

  QueueMessage msg;
  msg.cmd = X_CMD_SHOW_PPM;
  msg.ppm = tempPpm;
  if (updateQueue) xQueueSendToBack(updateQueue, (void*)&msg, pdMS_TO_TICKS(5));
}

void HUB75Matrix::dripTimer() {
  if (dripCurrentRow == dripFinalRow) {
    cyclicTimer->detach();
    cyclicTimerMode = TIMER_OFF;
    QueueMessage msg;
    msg.cmd = X_CMD_SHOW_PPM;
    msg.ppm = currentPpm;
    if (updateQueue) xQueueSendToBack(updateQueue, (void*)&msg, pdMS_TO_TICKS(5));
    return;
  }
  dripCurrentRow -= dripDirection;
  QueueMessage msg;
  msg.cmd = X_CMD_SHOW_DRIP;
  msg.ppm = dripDirection > 0 ? previousPpm : currentPpm;
  if (updateQueue) xQueueSendToBack(updateQueue, (void*)&msg, pdMS_TO_TICKS(5));
}

void HUB75Matrix::snakeTimer() {
  QueueMessage msg;
  msg.cmd = X_CMD_SHOW_PPM;
  msg.ppm = currentPpm;
  snakePos = snakePos + 1 % ((TANK_WIDTH + TANK_HEIGHT - 2) * 2);
  if (updateQueue) xQueueSendToBack(updateQueue, (void*)&msg, pdMS_TO_TICKS(5));
}

void HUB75Matrix::textTimer() {
  QueueMessage msg;
  msg.cmd = X_CMD_SHOW_TEXT;
  msg.ppm = currentPpm;
  if (scrollPosition == 0) scrollDirection = -1;
  else if (scrollPosition == -scrollWidth) scrollDirection = 1;
  scrollPosition += scrollDirection;
  if (updateQueue) xQueueSendToBack(updateQueue, (void*)&msg, pdMS_TO_TICKS(5));
}

void HUB75Matrix::hub75MatrixLoop(void* pvParameters) {
  HUB75Matrix* instance = (HUB75Matrix*)pvParameters;
  BaseType_t notified;
  QueueMessage msg;

  if (config.hub75R1 != 0 && config.hub75G1 != 0 && config.hub75B1 != 0 && config.hub75R2 != 0 && config.hub75G2 != 0 && config.hub75B2 != 0 && config.hub75ChA != 0
    && config.hub75ChB != 0 && config.hub75ChC != 0 && config.hub75ChD != 0 && config.hub75Clk != 0 && config.hub75Lat != 0 && config.hub75Oe) {
    HUB75_I2S_CFG::i2s_pins hub75Pins = { static_cast<int8_t>(config.hub75R1), static_cast<int8_t>(config.hub75G1), static_cast<int8_t>(config.hub75B1), static_cast<int8_t>(config.hub75R2),
      static_cast<int8_t>(config.hub75G2), static_cast<int8_t>(config.hub75B2), static_cast<int8_t>(config.hub75ChA), static_cast<int8_t>(config.hub75ChB), static_cast<int8_t>(config.hub75ChC),
      static_cast<int8_t>(config.hub75ChD), -1, static_cast<int8_t>(config.hub75Lat), static_cast<int8_t>(config.hub75Oe), static_cast<int8_t>(config.hub75Clk) };
    HUB75_I2S_CFG mxconfig(128, 64, 1, hub75Pins, HUB75_I2S_CFG::DP3246_SM5368, true, HUB75_I2S_CFG::HZ_15M, 1, true, 50, 4);
    instance->matrix = new MatrixPanel_I2S_DMA(mxconfig);
  }

  instance->GREEN = instance->matrix->color565(0, 255, 0);
  instance->YELLOW = instance->matrix->color565(255, 255, 0);
  instance->RED = instance->matrix->color565(255, 0, 0);

  instance->updateQueue = xQueueCreate(2, sizeof(struct QueueMessage));
  if (instance->updateQueue == NULL) {
    ESP_LOGE(TAG, "Queue creation failed!");
  }

  instance->matrix->begin();
  instance->matrix->setFont(&FreeMonoBold12pt7b);
  instance->matrix->setTextSize(instance->textSize);
  instance->matrix->setRotation(3);

  instance->matrix->setBrightness(5);
  for (byte i = 0; i < 3; i++) {
    instance->matrix->fillScreen(instance->matrix->color565(255 * (i <= 1 ? 1 : 0), 255 * (i >= 1 ? 1 : 0), 0));
    instance->matrix->flipDMABuffer();
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  instance->matrix->setBrightness(config.brightness);
  instance->matrix->fillScreen(0);
  instance->matrix->flipDMABuffer();
  instance->cyclicTimer = new Ticker();
  instance->snakeTicker = new Ticker();


  uint32_t lastModeChange = millis();
  while (1) {
    if (ENABLE_TEXT) {
      if (instance->displayMode == SHOW_TANK) {
        if (millis() - lastModeChange > TANK_INTERVAL) {
          instance->displayMode = SHOW_TEXT;
          lastModeChange = millis();
          instance->update(instance->currentPpm);
        }
      } else if (instance->displayMode == SHOW_TEXT) {
        if (millis() - lastModeChange > TEXT_INTERVAL) {
          instance->displayMode = SHOW_TANK;
          lastModeChange = millis();
          instance->update(instance->currentPpm);
        }
      }
    }
    notified = xQueueReceive(instance->updateQueue, &msg, pdMS_TO_TICKS(100));
    if (notified == pdPASS) {
      if (msg.cmd == X_CMD_SHOW_PPM) {
        instance->showTank(msg.ppm, false);
      } else if (msg.cmd == X_CMD_SHOW_DRIP) {
        instance->showTank(msg.ppm, true);
      } else if (msg.cmd == X_CMD_SHOW_TEXT) {
        instance->showText();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  vTaskDelete(NULL);
}

