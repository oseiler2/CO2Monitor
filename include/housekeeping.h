#ifndef _HOUSEKEEPING_H
#define _HOUSEKEEPING_H

#include <globals.h>
#include <Ticker.h>

namespace housekeeping {
  extern Ticker cyclicTimer;

  void doHousekeeping(void);
}

extern TaskHandle_t sensorsTask;
extern TaskHandle_t neopixelMatrixTask;

#endif
