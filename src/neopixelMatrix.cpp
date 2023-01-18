#include <neopixelMatrix.h>
#include <Fonts/TomThumb.h>
#include <configManager.h>

// Local logging tag
static const char TAG[] = __FILE__;

NeopixelMatrix::NeopixelMatrix(Model* _model, uint8_t _pin, uint8_t _columns, uint8_t _rows, uint8_t _layout) {
  this->model = _model;
  // default layout: NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE
  this->matrix = new Adafruit_NeoMatrix((int)_columns, (int)_rows, _pin, _layout, NEO_GRB + NEO_KHZ800);

  MATRIX_WIDTH = _columns;
  MATRIX_HEIGHT = _rows;
  NUMBER_OF_DOTS = MATRIX_WIDTH * MATRIX_HEIGHT;
  TANK_WIDTH = rotateTank ? MATRIX_HEIGHT : MATRIX_WIDTH;
  TANK_HEIGHT = rotateTank ? MATRIX_WIDTH : MATRIX_HEIGHT;

  LOWER_LIMIT = 450;
  UPPER_LIMIT = config.co2DarkRedThreshold;
  MID_POINT = config.co2YellowThreshold;
  RANGE = UPPER_LIMIT - LOWER_LIMIT;
  PPM_PER_DOT = (float)RANGE / (NUMBER_OF_DOTS);

  GREEN = matrix->Color(0, 255, 0);
  YELLOW = matrix->Color(255, 255, 0);
  RED = matrix->Color(255, 0, 0);

  updateQueue = xQueueCreate(2, sizeof(struct QueueMessage));
  if (updateQueue == NULL) {
    ESP_LOGE(TAG, "Queue creation failed!");
  }

  matrix->begin();
  matrix->setFont(&TomThumb);
  matrix->setTextWrap(false);

  matrix->setBrightness(5);
  for (byte i = 0; i < 3; i++) {
    matrix->fillScreen(matrix->Color(255 * (i <= 1 ? 1 : 0), 255 * (i >= 1 ? 1 : 0), 0));
    matrix->show();
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  matrix->setBrightness(config.brightness);
  matrix->fillScreen(0);
  matrix->show();
  cyclicTimer = new Ticker();
  snakeTicker = new Ticker();
}

NeopixelMatrix::~NeopixelMatrix() {
  if (this->matrix) delete matrix;
  if (this->cyclicTimer) delete cyclicTimer;
  if (this->snakeTicker) delete snakeTicker;
}

TaskHandle_t NeopixelMatrix::start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core) {
  xTaskCreatePinnedToCore(
    this->neopixelMatrixLoop,  // task function
    name,                      // name of task
    stackSize,                 // stack size of task
    this,                      // parameter of the task
    priority,                  // priority of the task
    &displayTask,              // task handle
    core);                     // CPU core
  return displayTask;
}

/**
 * Return the number of dots that represent the given PPM value, with ppm <= LOWER_LIMIT returning NUMBER_OF_DOTS (= full tank) and
 * ppm >= UPPER_LIMIT returning 0 (= empty tank)
 */
uint8_t NeopixelMatrix::ppmToDots(uint16_t ppm) {
  ppm = max(min(ppm, UPPER_LIMIT), LOWER_LIMIT);
  return NUMBER_OF_DOTS - min((uint8_t)floor((ppm - LOWER_LIMIT) / PPM_PER_DOT), NUMBER_OF_DOTS);
}

/**
 * Returns a colour for the given PPM value, with
 * ppm == LOWER_LIMIT returning GREEN,
 * ppm == mid point (LOWER_LIMIT, UPPER_LIMIT) returning YELLOW and
 * ppm == UPPER_LIMIT returning RED.
 * Values in between are interpolated.
 */
uint16_t NeopixelMatrix::ppmToColour(uint16_t ppm) {
  if (ppm < (MID_POINT)) {
    // green - yellow
    float d = float(max(ppm, LOWER_LIMIT) - LOWER_LIMIT) / float(MID_POINT - LOWER_LIMIT);
    return matrix->Color((uint8_t)(255.0 * d), 255, 0);
  } else {
    // yellow - red
    float d = float(min(ppm, config.co2RedThreshold) - (MID_POINT)) / float(config.co2RedThreshold - MID_POINT);
    return  matrix->Color(255, (uint8_t)(255.0 * (1.0 - d)), 0);
  }
}

void NeopixelMatrix::showText() {
  matrix->fillScreen(0);
  matrix->setTextColor(matrix->Color(255, 255, 255));//ppmToColour(currentPpm));
  matrix->setCursor(scrollPosition + (scrollWidth > 0 ? 1 : 0), MATRIX_HEIGHT);
  matrix->print(txt);
  matrix->show();
}

/**
 * Render the given PPM value on the matrix->
 */
void NeopixelMatrix::show(uint16_t ppm, bool showDrip) {
  uint16_t color = ppmToColour(ppm); //showDrip ? GREEN : GREEN; //ppmToColour(ppm);

  // takes about  1297 micros
  //  uint32_t start = micros();
  uint8_t dots = ppmToDots(ppm);

  matrix->clear();
  matrix->fillScreen(0);
  //  matrix->fillScreen(matrix->Color(255, 0, 255));
  uint8_t rows = min((uint8_t)floor(dots / TANK_WIDTH), TANK_HEIGHT);                   // number of full rows
  uint8_t remainder = dots % TANK_WIDTH;                                                  // number of additional dots
  //  ESP_LOGI(TAG, "show() ppm %u, dots %u, colums %u, remainder %u", ppm, dots, rows, remainder);

  if (rows > 0) matrix->fillRect(0, TANK_HEIGHT - rows, TANK_WIDTH, rows, color);        // render full rows
  if (remainder == 1) matrix->drawPixel(0, TANK_HEIGHT - rows - 1, color);                 // render additional dots (== 1)
  if (remainder > 1) matrix->drawFastHLine(0, TANK_HEIGHT - rows - 1, remainder, color);   // render additional dots (> 1)
  if (showDrip) matrix->drawPixel(dripColumn, TANK_HEIGHT - 1 - dripCurrentRow, dripDirection > 0 ? GREEN : color);

  // if snake(s) are active, draw snake(s) at current positions - dows not move the snake
  if (snakeTicker->active()) {
    for (uint8_t s = 0; s < NUMBER_OF_SNAKES; s++) {
      uint8_t snakeOffset = s * (((TANK_WIDTH + TANK_HEIGHT - 2) * 2)) / NUMBER_OF_SNAKES;
      for (uint8_t i = 0; i < SNAKE_LENGTH; i++) {
        uint8_t p = (snakePos + i + snakeOffset) % ((TANK_WIDTH + TANK_HEIGHT - 2) * 2);
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

  matrix->show();
  //  uint32_t end = micros();
  //  ESP_LOGI(TAG, "show() took %u us", end - start);
}

void NeopixelMatrix::update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
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
void NeopixelMatrix::update(uint16_t ppm) {
  // ESP_LOGD(TAG, "update: %u", ppm);
  previousPpm = currentPpm;
  currentPpm = ppm;

  if (displayMode == SHOW_TEXT) {
    matrix->setRotation(0);
    if (snakeTicker->active()) snakeTicker->detach();
    if (cyclicTimer->active()) cyclicTimer->detach();
    scrollPosition = 0;
    if (ppm == 0) {
      strcpy(txt, "---");
    } else {
      sprintf(txt, "%u", ppm);
    }
    int16_t x1, y1 = 0;
    uint16_t h = 0;
    matrix->getTextBounds(txt, 0, 0, &x1, &y1, &textWidth, &h);
    scrollWidth = textWidth - matrix->width();
    if (scrollWidth > 0) {
      scrollWidth += 2;
    }
    showText();
    // ESP_LOGD(TAG, "textTimer=> scrollPosition %i, textWidth %u, scrollWidth %i, scrollDirection %i", scrollPosition, textWidth, scrollWidth, scrollDirection);
    if (scrollWidth > 0) cyclicTimer->attach_ms(TEXT_TIMER_INTERVAL, +[](NeopixelMatrix* instance) { instance->textTimer(); }, this);

  } else if (displayMode == SHOW_TANK) {
    matrix->setRotation(rotateTank ? 1 : 0);

    uint16_t delta = previousPpm > currentPpm ? previousPpm - currentPpm : currentPpm - previousPpm;

    if (delta > 0 && delta < 2 * PPM_PER_DOT) {
      if (previousPpm > currentPpm) {
        // ppm decreasing
        dripDirection = 1;
        uint8_t dots = ppmToDots(ppm);
        dripCurrentRow = TANK_HEIGHT;
        dripColumn = (dots - 1) % TANK_WIDTH;
        dripFinalRow = min((uint8_t)floor((dots - 1) / TANK_WIDTH), TANK_HEIGHT);
        //      ESP_LOGD(TAG, "dripTimer=> dripColumn %u, dripFinalRow %u", dripColumn, dripFinalRow);
      } else {
        // ppm increasing
        dripDirection = -1;
        uint8_t dots = ppmToDots(previousPpm);
        dripCurrentRow = min((uint8_t)floor((dots - 1) / TANK_WIDTH), TANK_HEIGHT);
        dripColumn = (dots - 1) % TANK_WIDTH;
        dripFinalRow = TANK_HEIGHT - 1;
        //      ESP_LOGD(TAG, "dripTimer=> dripColumn %u, dripCurrentRow %u", dripColumn, dripCurrentRow);
      }
      if (cyclicTimer->active()) cyclicTimer->detach(); // cyclicTimer could be active on WAVE, hence detach
      cyclicTimer->attach_ms(DRIP_TIMER_INTERVAL, +[](NeopixelMatrix* instance) { instance->dripTimer(); }, this);
    } else {
      amplitude = 1;
      lastPpmUpdate = millis();
      if (cyclicTimer->active()) cyclicTimer->detach(); // cyclicTimer could be active on DRIP, hence detach
      cyclicTimer->attach_ms(WAVE_TIMER_INTERVAL, +[](NeopixelMatrix* instance) { instance->waveTimer(); }, this);
    }
    // turn on snake(s) if only 1 row or less is visible
    if (currentPpm > UPPER_LIMIT - (TANK_WIDTH * PPM_PER_DOT)) {
      if (!snakeTicker->active()) snakeTicker->attach_ms(SNAKE_TICKER_INTERVAL, +[](NeopixelMatrix* instance) { instance->snakeTimer(); }, this);
    } else {
      if (snakeTicker->active()) snakeTicker->detach();
    }
  }
}

void NeopixelMatrix::waveTimer() {
  if (amplitude < 0.05f) {
    cyclicTimer->detach();
    if (currentPpm > UPPER_LIMIT - (TANK_WIDTH * PPM_PER_DOT)) {
      // turn on snake(s) if only 1 row or less is visible
      if (!snakeTicker->active()) snakeTicker->attach_ms(SNAKE_TICKER_INTERVAL, +[](NeopixelMatrix* instance) { instance->snakeTimer(); }, this);
      return;
    }
  }
  uint16_t timePassed = millis() - lastPpmUpdate;
  float theta = float(timePassed) / 100; // 300
  uint16_t tempPpm = currentPpm - cos(theta) * (currentPpm - previousPpm) * (amplitude);
  //  ESP_LOGD(TAG, "timer() timePassed %u, theta %.1f, cos %.1f, amplitude %.2f, delta %d, tempPpm %u", timePassed, theta, cos(theta), amplitude, (currentPpm - previousPpm), tempPpm);
  amplitude -= amplitude * dampen;

  QueueMessage msg;
  msg.cmd = X_CMD_SHOW_PPM;
  msg.ppm = tempPpm;
  if (updateQueue) xQueueSendToBack(updateQueue, (void*)&msg, pdMS_TO_TICKS(5));
}

void NeopixelMatrix::dripTimer() {
  if (dripCurrentRow == dripFinalRow) {
    cyclicTimer->detach();
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

void NeopixelMatrix::snakeTimer() {
  QueueMessage msg;
  msg.cmd = X_CMD_SHOW_PPM;
  msg.ppm = currentPpm;
  snakePos = snakePos + 1 % ((TANK_WIDTH + TANK_HEIGHT - 2) * 2);
  if (updateQueue) xQueueSendToBack(updateQueue, (void*)&msg, pdMS_TO_TICKS(5));
}

void NeopixelMatrix::textTimer() {
  QueueMessage msg;
  msg.cmd = X_CMD_SHOW_TEXT;
  msg.ppm = currentPpm;
  if (scrollPosition == 0) scrollDirection = -1;
  else if (scrollPosition == -scrollWidth) scrollDirection = 1;
  scrollPosition += scrollDirection;
  if (updateQueue) xQueueSendToBack(updateQueue, (void*)&msg, pdMS_TO_TICKS(5));
}

void NeopixelMatrix::neopixelMatrixLoop(void* pvParameters) {
  NeopixelMatrix* instance = (NeopixelMatrix*)pvParameters;
  BaseType_t notified;
  QueueMessage msg;
  uint32_t lastModeChange = millis();
  while (1) {
    if (instance->displayMode == SHOW_TANK) {
      if (millis() - lastModeChange > 30000) {
        instance->displayMode = SHOW_TEXT;
        lastModeChange = millis();
        instance->update(instance->currentPpm);
      }
    } else if (instance->displayMode == SHOW_TEXT) {
      if (millis() - lastModeChange > 5000) {
        instance->displayMode = SHOW_TANK;
        lastModeChange = millis();
        instance->update(instance->currentPpm);
      }
    }
    notified = xQueueReceive(instance->updateQueue, &msg, pdMS_TO_TICKS(10));
    if (notified == pdPASS) {
      if (msg.cmd == X_CMD_SHOW_PPM) {
        instance->show(msg.ppm, false);
      } else if (msg.cmd == X_CMD_SHOW_DRIP) {
        instance->show(msg.ppm, true);
      } else if (msg.cmd == X_CMD_SHOW_TEXT) {
        instance->showText();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  vTaskDelete(NULL);
}