#include <neopixel.h>
#include <config.h>
#include <configManager.h>
#include <power.h>

// Local logging tag
static const char TAG[] = __FILE__;

#define BAT_BRIGHTNESS (uint8_t)15

Neopixel::Neopixel(Model* _model, uint8_t _pin, uint8_t numPixel, boolean reinitFromSleep) {
  this->model = _model;
  strip = new Adafruit_NeoPixel(numPixel, _pin, NEO_GRB + NEO_KHZ800);
  toggle = false;
  ticker = new Ticker();

  this->colourRed = this->strip->Color(255, 0, 0);
  this->colourYellow = this->strip->Color(255, 70, 0);
  this->colourGreen = this->strip->Color(0, 255, 0);
  this->colourPurple = this->strip->Color(255, 0, 255);
  this->colourOff = this->strip->Color(0, 0, 0);

  this->strip->begin();
  this->strip->setBrightness(Power::getRunMode() == RM_LOW ? min(BAT_BRIGHTNESS, config.brightness) : config.brightness);
  if (Power::getRunMode() == RM_FULL || !reinitFromSleep) {
    // https://stackoverflow.com/questions/60985496/arduino-esp8266-esp32-ticker-callback-class-member-function
    ticker->attach(0.3, +[](Neopixel* instance) { instance->timer(); }, this);
  }
  if (reinitFromSleep && Power::getRunMode() == RM_FULL) {
    // woke up from sleep, flush all 3 LEDs
    update(M_CONFIG_CHANGED, model->getStatus(), model->getStatus());
  }
  if (!reinitFromSleep) {
    this->strip->show(); // Initialize all pixels to 'off'
    fill(colourPurple);
    delay(250);
    fill(colourRed);
    delay(250);
    fill(colourYellow);
    delay(250);
    fill(colourGreen);
    delay(250);
    fill(colourOff);
  }
}

Neopixel::~Neopixel() {
  if (this->ticker) delete ticker;
  if (this->strip) delete strip;
}

void Neopixel::fill(uint32_t c) {
  uint8_t limit = Power::getRunMode() == RM_LOW ? 1 : this->strip->numPixels();
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

void Neopixel::prepareToSleep() {
  this->strip->setBrightness(0);
  for (uint16_t i = 0; i < this->strip->numPixels(); i++) {
    this->strip->setPixelColor(i, colourOff);
  }
  this->strip->show();
  if (config.sleepModeOledLed == SLEEP_OLED_OFF_LED_ON || config.sleepModeOledLed == SLEEP_OLED_ON_LED_ON) {
    this->strip->setBrightness(min(BAT_BRIGHTNESS, config.brightness));
    uint32_t c;
    switch (model->getStatus()) {
      case GREEN:
        c = colourGreen;
        break;
      case YELLOW:
        c = colourYellow;
        break;
      case RED:
        c = colourRed;
        break;
      case DARK_RED:
        c = colourPurple;
        break;
      default:
        c = colourOff;
        break;
    }
    this->strip->setPixelColor(0, c);
    this->strip->show();
  }
  ticker->detach();
#ifdef NEO_23_EN
  digitalWrite(NEO_23_EN, LOW);
#endif
}

void Neopixel::update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
  if (oldStatus == newStatus && !(mask & M_CONFIG_CHANGED)) return;
  if (mask & M_CONFIG_CHANGED) this->strip->setBrightness(Power::getRunMode() == RM_LOW ? min(BAT_BRIGHTNESS, config.brightness) : config.brightness);
  if (newStatus == OFF) {
    fill(colourGreen);
  } else if (newStatus == GREEN) {
    fill(colourGreen);
  } else if (newStatus == YELLOW) {
    fill(colourYellow);
  } else if (newStatus == RED) {
    fill(colourRed);
  } else if (newStatus == DARK_RED) {
    if (Power::getRunMode() == RM_LOW || oldStatus != newStatus)
      fill(colourPurple);
  }
}

void Neopixel::timer() {
  this->toggle = !(this->toggle);
  if (model->getStatus() == DARK_RED) {
    if (toggle)
      fill(colourPurple);
    else
      fill(colourOff);
  }
}