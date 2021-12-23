#include <trafficLight.h>
#include <config.h>
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

#define pwmFreq           10000
#define pwmChannelRed     0
#define pwmChannelYellow  1
#define pwmChannelGreen   2
#define pwmResolution     8
#define pwmMax            20

  ledcSetup(pwmChannelRed, pwmFreq, pwmResolution);
  ledcSetup(pwmChannelYellow, pwmFreq, pwmResolution);
  ledcSetup(pwmChannelGreen, pwmFreq, pwmResolution);
  ledcAttachPin(pinRed, pwmChannelRed);
  ledcAttachPin(pinYellow, pwmChannelYellow);
  ledcAttachPin(pinGreen, pwmChannelGreen);
  ledcWrite(pwmChannelRed, 0);
  ledcWrite(pwmChannelYellow, 0);
  ledcWrite(pwmChannelGreen, 0);

  pinMode(pinRed, OUTPUT);
  ledcWrite(pwmChannelRed, pwmMax);
  delay(500);
  pinMode(pinYellow, OUTPUT);
  ledcWrite(pwmChannelYellow, pwmMax);
  delay(500);
  pinMode(pinGreen, OUTPUT);
  ledcWrite(pwmChannelGreen, pwmMax);
  delay(500);
  ledcWrite(pwmChannelRed, 0);
  ledcWrite(pwmChannelYellow, 0);
  ledcWrite(pwmChannelGreen, 0);
  delay(500);
  this->status = UNDEFINED;
}

TrafficLight::~TrafficLight() {
  if (this->cyclicTimer) delete cyclicTimer;
}

void TrafficLight::update() {
  if (model->getCo2() < config.yellowThreshold && this->status != GREEN) {
    this->status = GREEN;
    ledcWrite(pwmChannelRed, 0);
    ledcWrite(pwmChannelYellow, 0);
    ledcWrite(pwmChannelGreen, pwmMax);
  } else if (model->getCo2() < config.redThreshold && this->status != YELLOW) {
    this->status = YELLOW;
    ledcWrite(pwmChannelRed, 0);
    ledcWrite(pwmChannelYellow, pwmMax);
    ledcWrite(pwmChannelGreen, 0);
  } else if (model->getCo2() < config.darkRedThreshold && this->status != RED) {
    this->status = RED;
    ledcWrite(pwmChannelRed, pwmMax);
    ledcWrite(pwmChannelYellow, 0);
    ledcWrite(pwmChannelGreen, 0);
  } else if (model->getCo2() >= config.darkRedThreshold && this->status != DARK_RED) {
    this->status = DARK_RED;
    ledcWrite(pwmChannelRed, pwmMax);
    ledcWrite(pwmChannelYellow, 0);
    ledcWrite(pwmChannelGreen, 0);
  }
}

void TrafficLight::timer() {
  if (this->status == DARK_RED) {
    if (ledcRead(pwmChannelRed) == 0)
      ledcWrite(pwmChannelRed, pwmMax);
    else
      ledcWrite(pwmChannelRed, 0);
  }
}