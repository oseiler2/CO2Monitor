#include <neopixel.h>
#include <config.h>
#include <configManager.h>

// Local logging tag
static const char TAG[] = __FILE__;

Neopixel::Neopixel(Model* _model, uint8_t _pin, uint8_t numPixel) {
  this->model = _model;
  strip = new Adafruit_NeoPixel(numPixel, _pin, NEO_GRB + NEO_KHZ800);
  toggle = false;
  cyclicTimer = new Ticker();

  this->colourRed = this->strip->Color(255, 0, 0);
  this->colourYellow = this->strip->Color(255, 70, 0);
  this->colourGreen = this->strip->Color(0, 255, 0);
  this->colourPurple = this->strip->Color(255, 0, 255);
  this->colourOff = this->strip->Color(0, 0, 0);

  // https://stackoverflow.com/questions/60985496/arduino-esp8266-esp32-ticker-callback-class-member-function
  cyclicTimer->attach(0.3, +[](Neopixel* instance) { instance->timer(); }, this);

  this->strip->begin();
  this->strip->setBrightness(config.brightness);
  this->strip->show(); // Initialize all pixels to 'off'
  fill(colourRed);
  delay(250);
  fill(colourYellow);
  delay(250);
  fill(colourGreen);
  delay(250);
  fill(colourOff);
}

Neopixel::~Neopixel() {
  if (this->cyclicTimer) delete cyclicTimer;
  if (this->strip) delete strip;
}

void Neopixel::fill(uint32_t c) {
  for (uint16_t i = 0; i < this->strip->numPixels(); i++) {
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
  if (oldStatus == newStatus && !(mask & M_CONFIG_CHANGED)) return;
  if (mask & M_CONFIG_CHANGED) this->strip->setBrightness(config.brightness);
  if (newStatus == OFF) {
    fill(colourOff);
  } else if (newStatus == GREEN) {
    fill(colourGreen);
  } else if (newStatus == YELLOW) {
    fill(colourYellow);
  } else if (newStatus == RED) {
    fill(colourRed);
  } else if (newStatus == DARK_RED) {
    fill(colourRed);
  }
}

void Neopixel::timer() {
  this->toggle = !(this->toggle);
  if (model->getStatus() == DARK_RED) {
    if (toggle)
      fill(colourRed);
    else
      fill(colourOff);
  }
}