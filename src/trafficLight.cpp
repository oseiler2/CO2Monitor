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

  pinMode(pinRed, OUTPUT);
  digitalWrite(pinRed, HIGH);
  delay(500);
  pinMode(pinYellow, OUTPUT);
  digitalWrite(pinYellow, HIGH);
  delay(500);
  pinMode(pinGreen, OUTPUT);
  digitalWrite(pinGreen, HIGH);
  delay(500);
  digitalWrite(pinRed, LOW);
  digitalWrite(pinGreen, LOW);
  digitalWrite(pinYellow, LOW);
  delay(500);
  this->status = UNDEFINED;
}

TrafficLight::~TrafficLight() {
  if (this->cyclicTimer) delete cyclicTimer;
}

void TrafficLight::update() {
  if (model->getCo2() < config.yellowThreshold) {
    this->status = GREEN;
  } else if (model->getCo2() < config.redThreshold) {
    this->status = YELLOW;
  } else if (model->getCo2() < config.darkRedThreshold) {
    this->status = RED;
  } else {
    this->status = DARK_RED;
  }

  if (this->status == GREEN) {
    digitalWrite(pinRed, LOW);
    digitalWrite(pinYellow, LOW);
    digitalWrite(pinGreen, HIGH);
  } else if (this->status == YELLOW) {
    digitalWrite(pinRed, LOW);
    digitalWrite(pinYellow, HIGH);
    digitalWrite(pinGreen, LOW);
  } else if (this->status == RED) {
    digitalWrite(pinRed, HIGH);
    digitalWrite(pinYellow, LOW);
    digitalWrite(pinGreen, LOW);
  } else if (this->status == DARK_RED) {
    digitalWrite(pinRed, HIGH);
    digitalWrite(pinYellow, LOW);
    digitalWrite(pinGreen, LOW);
  } else {
    digitalWrite(pinRed, LOW);
    digitalWrite(pinYellow, LOW);
    digitalWrite(pinGreen, LOW);
  }
}

void TrafficLight::timer() {
  if (this->status == DARK_RED) {
    digitalWrite(pinRed, !digitalRead(pinRed));
  }
}