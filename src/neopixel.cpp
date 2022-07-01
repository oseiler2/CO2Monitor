#include <neopixel.h>
#include <config.h>
#include <configManager.h>

Neopixel::Neopixel(Model* _model, uint8_t _pin, uint8_t numPixel) {
  this->model = _model;
  strip = new Adafruit_NeoPixel(numPixel, _pin, NEO_GRB + NEO_KHZ800);
  toggle = false;
  cyclicTimer = new Ticker();

  // https://stackoverflow.com/questions/60985496/arduino-esp8266-esp32-ticker-callback-class-member-function
  cyclicTimer->attach(0.3, +[](Neopixel* instance) { instance->timer(); }, this);

  this->strip->begin();
  this->strip->setBrightness(config.brightness);
  this->strip->show(); // Initialize all pixels to 'off'
  fill(this->strip->Color(255, 0, 0)); // Red
  delay(500);
  fill(this->strip->Color(255, 70, 0)); // Amber
  delay(500);
  fill(this->strip->Color(0, 255, 0)); // Green
  delay(500);
  fill(this->strip->Color(0, 0, 0)); // off
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

void Neopixel::update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
  if (oldStatus == newStatus && !(mask & M_CONFIG_CHANGED)) return;
  if (mask & M_CONFIG_CHANGED) this->strip->setBrightness(config.brightness);
  if (newStatus == GREEN) {
    fill(this->strip->Color(0, 255, 0)); // Green
  } else if (newStatus == YELLOW) {
    fill(this->strip->Color(255, 70, 0)); // Amber
  } else if (newStatus == RED) {
    fill(this->strip->Color(255, 0, 0)); // Red
  } else if (newStatus == DARK_RED) {
    fill(this->strip->Color(255, 0, 0)); // Red
  }
}

void Neopixel::timer() {
  this->toggle = !(this->toggle);
  if (model->getStatus() == DARK_RED) {
    if (toggle)
      fill(this->strip->Color(255, 0, 0)); // Red
    else
      fill(this->strip->Color(0, 0, 0));
  }
}