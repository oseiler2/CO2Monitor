#include <hub75.h>
#include <config.h>
#include <configManager.h>

#include <smileys.h>
#include <digits.h>
#include <message.h>

// Local logging tag
static const char TAG[] = __FILE__;

#define USE_FASTLINES

HUB75::HUB75(Model* _model) {
  this->model = _model;
  if (config.hub75R1 != 0 && config.hub75G1 != 0 && config.hub75B1 != 0 && config.hub75R2 != 0 && config.hub75G2 != 0 && config.hub75B2 != 0 && config.hub75ChA != 0
    && config.hub75ChB != 0 && config.hub75ChC != 0 && config.hub75ChD != 0 && config.hub75Clk != 0 && config.hub75Lat != 0 && config.hub75Oe) {
    HUB75_I2S_CFG::i2s_pins hub75Pins = { static_cast<int8_t>(config.hub75R1), static_cast<int8_t>(config.hub75G1), static_cast<int8_t>(config.hub75B1), static_cast<int8_t>(config.hub75R2),
      static_cast<int8_t>(config.hub75G2), static_cast<int8_t>(config.hub75B2), static_cast<int8_t>(config.hub75ChA), static_cast<int8_t>(config.hub75ChB), static_cast<int8_t>(config.hub75ChC),
      static_cast<int8_t>(config.hub75ChD), -1, static_cast<int8_t>(config.hub75Lat), static_cast<int8_t>(config.hub75Oe), static_cast<int8_t>(config.hub75Clk) };
    HUB75_I2S_CFG mxconfig(64, 32, 1, hub75Pins, HUB75_I2S_CFG::FM6126A, false, HUB75_I2S_CFG::HZ_15M, 1, true, 50, 6);
    this->matrix = new MatrixPanel_I2S_DMA(mxconfig);
  }
  matrix->begin();
  matrix->setRotation(3);
  matrix->setBrightness8(config.brightness);

  cyclicTimer = new Ticker();
  // https://arduino.stackexchange.com/questions/81123/using-lambdas-as-callback-functions
  //  cyclicTimer->attach<typeof this>(1, [](typeof this p) { p->timer(); },
  //  this);

  // https://stackoverflow.com/questions/60985496/arduino-esp8266-esp32-ticker-callback-class-member-function
  cyclicTimer->attach(0.3, +[](HUB75* instance) { instance->timer(); }, this);
  this->toggle = false;
  ESP_LOGD(TAG, "HUB75 initialised");
}

HUB75::~HUB75() {
  if (this->matrix) delete matrix;
}

void HUB75::stopDMA() {
  if (this->matrix) matrix->stopDMAoutput();
}

void HUB75::update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
  if (mask & M_CONFIG_CHANGED) matrix->setBrightness8(config.brightness);
  //  ESP_LOGD(TAG, "HUB75 update: %i => %i, co2: %u, mask:%x", oldStatus, newStatus, model->getCo2(), mask);
  if (oldStatus != newStatus) {
    // only redraw smiley on status change
    matrix->fillRect(0, 0, 32, 32, 0);
    if (newStatus > 0 && newStatus <= 4) {
      matrix->drawRGBBitmap(0, 0, smileys[newStatus - 1], 32, 32);
    }
    // show message
    if (newStatus > 0 && newStatus <= 4) {
      matrix->fillRect(0, 48, 32, 16, 0);
      matrix->drawBitmap(0, 48, messages[newStatus - 1], 32, 16, matrix->color565(255, 255, 255));
    }
  }

  if (mask & M_CO2) {
    // clear co2 reading and message
    matrix->fillRect(0, 33, 32, 16, 0);
    // show co2 reading
    if (model->getCo2() > 9999) {
      matrix->drawBitmap(0, 35, digits[9], 8, 10, matrix->color565(255, 255, 255));
      matrix->drawBitmap(8, 35, digits[9], 8, 10, matrix->color565(255, 255, 255));
      matrix->drawBitmap(16, 35, digits[9], 8, 10, matrix->color565(255, 255, 255));
      matrix->drawBitmap(24, 35, digits[9], 8, 10, matrix->color565(255, 255, 255));
    } else {
      if (model->getCo2() > 999)
        matrix->drawBitmap(0, 35, digits[(uint16_t)(model->getCo2() / 1000) % 10], 8, 10, matrix->color565(255, 255, 255));
      if (model->getCo2() > 99)
        matrix->drawBitmap(8, 35, digits[(uint16_t)(model->getCo2() / 100) % 10], 8, 10, matrix->color565(255, 255, 255));
      if (model->getCo2() > 9)
        matrix->drawBitmap(16, 35, digits[(uint16_t)(model->getCo2() / 10) % 10], 8, 10, matrix->color565(255, 255, 255));
      if (model->getCo2() > 0)
        matrix->drawBitmap(24, 35, digits[model->getCo2() % 10], 8, 10, matrix->color565(255, 255, 255));
    }
  }
}

void HUB75::timer() {
  if (model->getStatus() == DARK_RED) {
    if (toggle)
      matrix->drawRGBBitmap(0, 0, smileys[model->getStatus() - 1], 32, 32);
    else
      matrix->fillRect(0, 0, 32, 32, 0);
    toggle = !toggle;
  }
}
