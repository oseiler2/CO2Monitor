#include <neopixel.h>
#include <config.h>
#include <configManager.h>
#include <power.h>

// Local logging tag
static const char TAG[] = __FILE__;

#define BAT_BRIGHTNESS 10

Neopixel::Neopixel(Model* _model, uint8_t _pin, uint8_t numPixel, boolean reinitFromSleep) {
  this->model = _model;
  strip = new Adafruit_NeoPixel(numPixel, _pin, NEO_GRB + NEO_KHZ800);
  toggle = false;
  cyclicTimer = new Ticker();

  this->colourRed = this->strip->Color(255, 0, 0);
  this->colourYellow = this->strip->Color(255, 70, 0);
  this->colourGreen = this->strip->Color(0, 255, 0);
  this->colourOff = this->strip->Color(0, 0, 0);

  this->strip->begin();
  this->strip->setBrightness(Power::getPowerMode() == BATTERY ? BAT_BRIGHTNESS : config.brightness);
  if (!reinitFromSleep) {
    // https://stackoverflow.com/questions/60985496/arduino-esp8266-esp32-ticker-callback-class-member-function
    cyclicTimer->attach(0.3, +[](Neopixel* instance) { instance->timer(); }, this);

    this->strip->show(); // Initialize all pixels to 'off'
    fill(colourRed);
    delay(500);
    fill(colourYellow);
    delay(500);
    fill(colourGreen);
    delay(500);
    fill(colourOff);
  }
}

Neopixel::~Neopixel() {
  if (this->cyclicTimer) delete cyclicTimer;
  if (this->strip) delete strip;
}

void Neopixel::fill(uint32_t c) {
  uint8_t limit = Power::getPowerMode() == BATTERY ? 1 : this->strip->numPixels();
  for (uint16_t i = 0; i < limit; i++) {
    this->strip->setPixelColor(i, c);
  }
  this->strip->show();
}

void Neopixel::off() {
  this->strip->setBrightness(0);
  for (uint16_t i = 0; i < this->strip->numPixels(); i++) {
    this->strip->setPixelColor(i, colourOff);
  }
  this->strip->show();
}

void Neopixel::update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
  if (mask && M_POWER_MODE) {
    switch (Power::getPowerMode()) {
      case USB:
        this->strip->setBrightness(config.brightness);
        break;
      case BATTERY:
        off();
        this->strip->setBrightness(BAT_BRIGHTNESS);
        break;
      default: break;
    }
  }
  if (oldStatus == newStatus && !(mask & M_CONFIG_CHANGED) && !(mask && M_POWER_MODE)) return;
  if (mask & M_CONFIG_CHANGED) this->strip->setBrightness(Power::getPowerMode() == BATTERY ? BAT_BRIGHTNESS : config.brightness);
  if (newStatus == GREEN) {
    fill(this->colourGreen); // Green
  } else if (newStatus == YELLOW) {
    fill(colourYellow); // Amber
  } else if (newStatus == RED) {
    fill(colourRed); // Red
  } else if (newStatus == DARK_RED) {
    fill(colourRed); // Red
  }
}

void Neopixel::timer() {
  this->toggle = !(this->toggle);
  if (model->getStatus() == DARK_RED) {
    if (toggle)
      fill(colourRed); // Red
    else
      fill(colourOff);
  }
}