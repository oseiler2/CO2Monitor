#ifndef _HOUSEKEEPING_H
#define _HOUSEKEEPING_H

#include "globals.h"
#include <Ticker.h>

namespace housekeeping {
  extern Ticker cyclicTimer;

  void doHousekeeping(void);
}

extern TaskHandle_t scd30Task;
extern TaskHandle_t scd40Task;
extern TaskHandle_t bme680Task;

#endif
