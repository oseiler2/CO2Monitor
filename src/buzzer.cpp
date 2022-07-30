#include <buzzer.h>

// Local logging tag
static const char TAG[] = __FILE__;

#define PWM_FREQ_BUZZER           2000
#define PWM_RESOULTION_BUZZER     8
#define BUZZER_DUTY               128

Buzzer::Buzzer(Model* _model, uint8_t _buzzerPin) {
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
  ledcWrite(PWM_CHANNEL_BUZZER, BUZZER_DUTY);
  delay(300);
  ledcSetup(PWM_CHANNEL_BUZZER, PWM_FREQ_BUZZER + 500, PWM_RESOULTION_BUZZER);
  delay(300);
  ledcSetup(PWM_CHANNEL_BUZZER, PWM_FREQ_BUZZER + 1000, PWM_RESOULTION_BUZZER);
  delay(300);
  ledcWrite(PWM_CHANNEL_BUZZER, 0);
  ledcSetup(PWM_CHANNEL_BUZZER, PWM_FREQ_BUZZER, PWM_RESOULTION_BUZZER);
}

Buzzer::~Buzzer() {
  if (this->cyclicTimer) delete cyclicTimer;
}

void Buzzer::update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
  if (oldStatus == newStatus && !(mask & M_CONFIG_CHANGED)) return;
  if (newStatus == GREEN) {
    ledcWrite(PWM_CHANNEL_BUZZER, 0);
  } else if (newStatus == YELLOW) {
    ledcWrite(PWM_CHANNEL_BUZZER, 0);
  } else if (newStatus == RED) {
    ledcWrite(PWM_CHANNEL_BUZZER, 0);
  } else if (newStatus == DARK_RED) {
    buzzCtr = DARK_RED_BUZZES;
  }
}

void Buzzer::timer() {
  if (model->getStatus() == DARK_RED) {
    if (ledcRead(PWM_CHANNEL_BUZZER) == 0) {
      if (buzzCtr > 0) ledcWrite(PWM_CHANNEL_BUZZER, BUZZER_DUTY);
    } else {
      ledcWrite(PWM_CHANNEL_BUZZER, 0);
      if (buzzCtr > 0) buzzCtr--;
    }
  }
}