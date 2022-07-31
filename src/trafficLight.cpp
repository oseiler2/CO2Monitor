#include <trafficLight.h>
#include <configManager.h>

TrafficLight::TrafficLight(Model* _model, uint8_t _pinRed, uint8_t _pinYellow, uint8_t _pinGreen) {
  this->model = _model;
  this->pinRed = _pinRed;
  this->pinYellow = _pinYellow;
  this->pinGreen = _pinGreen;
  cyclicTimer = new Ticker();
  // https://arduino.stackexchange.com/questions/81123/using-lambdas-as-callback-functions
  //  cyclicTimer->attach<typeof this>(1, [](typeof this p) { p->timer(); },
  //  this);

  // https://stackoverflow.com/questions/60985496/arduino-esp8266-esp32-ticker-callback-class-member-function
  cyclicTimer->attach(0.3, +[](TrafficLight* instance) { instance->timer(); }, this);


  /*
  CLK src         max freq  max res
  80 MHz APB_CLK    1 KHz   16 bit
  80 MHz APB_CLK    5 KHz   14 bit
  80 MHz APB_CLK   10 KHz   13 bit
  8 MHz RTC8M_CLK   1 KHz   13 bit
  8 MHz RTC8M_CLK   8 KHz   10 bit
  1 MHz REF_TICK    1 KHz   10 bit
  */

#define PWM_FREQ_LEDS             10000
#define PWM_RESOLUTION_LEDS       8

  pinMode(pinRed, OUTPUT);
  pinMode(pinYellow, OUTPUT);
  pinMode(pinGreen, OUTPUT);
  ledcSetup(PWM_CHANNEL_LEDS, PWM_FREQ_LEDS, PWM_RESOLUTION_LEDS);
  ledcWrite(PWM_CHANNEL_LEDS, 0);

  ledcAttachPin(pinRed, PWM_CHANNEL_LEDS);
  ledcWrite(PWM_CHANNEL_LEDS, config.brightness);
  delay(500);
  ledcDetachPin(pinRed);
  ledcAttachPin(pinYellow, PWM_CHANNEL_LEDS);
  ledcWrite(PWM_CHANNEL_LEDS, config.brightness);
  delay(500);
  ledcDetachPin(pinYellow);
  ledcAttachPin(pinGreen, PWM_CHANNEL_LEDS);
  ledcWrite(PWM_CHANNEL_LEDS, config.brightness);
  delay(500);
  ledcDetachPin(pinGreen);
  ledcWrite(PWM_CHANNEL_LEDS, 0);
}

TrafficLight::~TrafficLight() {
  if (this->cyclicTimer) delete cyclicTimer;
}

void TrafficLight::update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
  if (oldStatus == newStatus && !(mask & M_CONFIG_CHANGED)) return;
  if (newStatus == GREEN) {
    ledcAttachPin(pinGreen, PWM_CHANNEL_LEDS);
    ledcDetachPin(pinYellow);
    ledcDetachPin(pinRed);
    ledcWrite(PWM_CHANNEL_LEDS, config.brightness);
  } else if (newStatus == YELLOW) {
    ledcDetachPin(pinGreen);
    ledcAttachPin(pinYellow, PWM_CHANNEL_LEDS);
    ledcDetachPin(pinRed);
    ledcWrite(PWM_CHANNEL_LEDS, config.brightness);
  } else if (newStatus == RED) {
    ledcDetachPin(pinGreen);
    ledcDetachPin(pinYellow);
    ledcAttachPin(pinRed, PWM_CHANNEL_LEDS);
    ledcWrite(PWM_CHANNEL_LEDS, config.brightness);
  } else if (newStatus == DARK_RED) {
    ledcDetachPin(pinGreen);
    ledcDetachPin(pinYellow);
    ledcAttachPin(pinRed, PWM_CHANNEL_LEDS);
  }
}

void TrafficLight::timer() {
  if (model->getStatus() == DARK_RED) {
    if (ledcRead(PWM_CHANNEL_LEDS) == 0)
      ledcWrite(PWM_CHANNEL_LEDS, config.brightness);
    else
      ledcWrite(PWM_CHANNEL_LEDS, 0);
  }
}