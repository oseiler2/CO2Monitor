#include <config.h>
#include <Arduino.h>
#include <lcd.h>
#include <i2c.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

#define FONT_64 &FreeMonoBold24pt7b
#define FONT_32 &FreeMonoBold18pt7b
#define FONT_9 &FreeMono9pt7b

LCD::LCD(TwoWire* _wire, Model* _model) {
  this->model = _model;
  display = new Adafruit_SSD1306(128, SSD1306_HEIGHT, _wire, -1, 50000, I2C_CLK);

  if (!I2C::takeMutex(portMAX_DELAY)) return;
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  this->display->begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADR);  // initialize with the I2C addr 0x3C (for the 128x32)

  // Clear the buffer.
  this->display->clearDisplay();
  this->display->display();
  this->display->setTextColor(WHITE);
  I2C::giveMutex();
}

LCD::~LCD() {
  if (this->display) delete display;
};

void LCD::updateMessage(char const* msg) {
  if (!I2C::takeMutex(pdMS_TO_TICKS(1000))) return;
  this->display->writeFillRect(0, SSD1306_HEIGHT == 32 ? 24 : 0, 128, 8, BLACK);
  this->display->setCursor(0, SSD1306_HEIGHT == 32 ? 24 : 0);
  this->display->setFont(NULL);
  this->display->setTextSize(1);
  this->display->printf("%-21s", msg);
  this->display->display();
  I2C::giveMutex();
}

void LCD::update(uint16_t mask) {
  if (!mask || M_CO2) return;
  if (!I2C::takeMutex(pdMS_TO_TICKS(1000))) return;
  if (model->getCo2() != 0) {
    // 8-24 vs 12-40
    this->display->writeFillRect(4, SSD1306_HEIGHT == 32 ? 0 : 12, 120, SSD1306_HEIGHT == 32 ? 32 : 40, BLACK);
    this->display->setTextSize(1);
    if (SSD1306_HEIGHT == 32) {
      this->display->setFont(FONT_32);
      this->display->setCursor(4, 22);
    } else {
      this->display->setFont(FONT_64);
      this->display->setCursor(4, 46);
    }

    if (model->getCo2() == 0) {
      this->display->print("----");
    } else {
      this->display->printf("%4u", model->getCo2());
    }
    this->display->setCursor(this->display->getCursorX(), this->display->getCursorY() - 3);
    this->display->setFont(FONT_9);
    this->display->setTextSize(1);
    this->display->print("ppm");
  }

  this->display->writeFillRect(0, SSD1306_HEIGHT == 32 ? 24 : 56, 128, 8, BLACK);
  this->display->setFont(NULL);
  this->display->setTextSize(1);
  this->display->setCursor(0, SSD1306_HEIGHT == 32 ? 24 : 56);
  this->display->printf("temp: %3.1f  hum: %2.0f %%", model->getTemperature(), model->getHumidity());
  this->display->display();
  I2C::giveMutex();
}

