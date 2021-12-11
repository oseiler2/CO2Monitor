#ifndef _I2C_H
#define _I2C_H

#include "globals.h"

namespace I2C {
#define SSD1306_I2C_ADR 0x3C
#define SCD30_I2C_ADR 0x61
#define SCD40_I2C_ADR 0x62
#define BME680_I2C_ADR 0x77

  void initI2C();

  boolean takeMutex(TickType_t blockTime);
  void giveMutex();

  boolean lcdPresent();
  boolean scd30Present();
  boolean scd40Present();
  boolean bme680Present();
}
#endif