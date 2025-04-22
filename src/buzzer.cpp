#include <buzzer.h>

#include <power.h>
#include <configManager.h>

// Local logging tag
static const char TAG[] = "Buzzer";

#if HAS_BUZZER
#define PWM_FREQ_BUZZER           2000
#define PWM_RESOULTION_BUZZER     8
#define BUZZER_DUTY               128

Buzzer::Buzzer(Model* _model, uint8_t _buzzerPin, boolean reinitFromSleep) {
  this->model = _model;
  this->buzzerPin = _buzzerPin;
  cyclicTimer = new Ticker();

  // https://arduino.stackexchange.com/questions/81123/using-lambdas-as-callback-functions
  //  cyclicTimer->attach<typeof this>(1, [](typeof this p) { p->timer(); },
  //  this);

  // https://stackoverflow.com/questions/60985496/arduino-esp8266-esp32-ticker-callback-class-member-function
  cyclicTimer->attach(0.3, +[](Buzzer* instance) { instance->timer(); }, this);

  /*
  CLK src         max freq  max res
  80 MHz APB_CLK    1 KHz   16 bit
  80 MHz APB_CLK    5 KHz   14 bit
  80 MHz APB_CLK   10 KHz   13 bit
  8 MHz RTC8M_CLK   1 KHz   13 bit
  8 MHz RTC8M_CLK   8 KHz   10 bit
  1 MHz REF_TICK    1 KHz   10 bit
  */

  pinMode(this->buzzerPin, OUTPUT);
  ledcSetup(PWM_CHANNEL_BUZZER, PWM_FREQ_BUZZER, PWM_RESOULTION_BUZZER);
  ledcAttachPin(this->buzzerPin, PWM_CHANNEL_BUZZER);
  if (!reinitFromSleep && config.buzzerMode != BUZ_OFF) {
    ledcWrite(PWM_CHANNEL_BUZZER, BUZZER_DUTY);
    delay(300);
    ledcSetup(PWM_CHANNEL_BUZZER, PWM_FREQ_BUZZER + 500, PWM_RESOULTION_BUZZER);
    delay(300);
    ledcSetup(PWM_CHANNEL_BUZZER, PWM_FREQ_BUZZER + 1000, PWM_RESOULTION_BUZZER);
    delay(300);
  }
  ledcWrite(PWM_CHANNEL_BUZZER, 0);
}

Buzzer::~Buzzer() {
  if (this->cyclicTimer) delete cyclicTimer;
}

void Buzzer::alert() {
  beep(5);
}

void Buzzer::beep(uint8_t n) {
  if (config.buzzerMode == BUZ_OFF) return;
  for (uint8_t i = 0; i < n;i++) {
    ledcWrite(PWM_CHANNEL_BUZZER, BUZZER_DUTY);
    delay(50);
    ledcWrite(PWM_CHANNEL_BUZZER, 0);
    delay(50);
  }
}

void Buzzer::update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
  if (!(mask & M_CO2) || (config.buzzerMode != BUZ_ALWAYS && oldStatus == newStatus)) return;
  if (newStatus == GREEN) {
    ledcWrite(PWM_CHANNEL_BUZZER, 0);
    beep(1);
  } else if (newStatus == YELLOW) {
    ledcWrite(PWM_CHANNEL_BUZZER, 0);
    beep(2);
  } else if (newStatus == RED) {
    ledcWrite(PWM_CHANNEL_BUZZER, 0);
    beep(3);
  } else if (newStatus == DARK_RED) {
    if (Power::getRunMode() == RM_LOW) {
      beep(4);
    } else {
      buzzCtr = DARK_RED_BUZZES;
    }
  }
}

void Buzzer::timer() {
  if (model->getStatus() == DARK_RED && config.buzzerMode != BUZ_OFF) {
    if (ledcRead(PWM_CHANNEL_BUZZER) == 0) {
      if (buzzCtr > 0) ledcWrite(PWM_CHANNEL_BUZZER, BUZZER_DUTY);
    } else {
      ledcWrite(PWM_CHANNEL_BUZZER, 0);
      if (buzzCtr > 0) buzzCtr--;
    }
  }
}
#endif
