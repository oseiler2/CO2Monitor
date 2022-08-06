#ifndef _POWER_H
#define _POWER_H

#include <globals.h>
#include <model.h>

typedef enum {
  PM_UNDEFINED = 0,
  USB,
  BATTERY
} PowerMode;


typedef enum {
  RR_UNDEFINED = 0,
  FULL_RESET,
  WAKE_FROM_SLEEPTIMER,
  WAKE_FROM_BUTTON
} ResetReason;

typedef void (*powerModeChangedEvt_t)(PowerMode mode);

extern Model* model;

namespace Power {
  ResetReason afterReset();
  void deepSleep(uint64_t durationInSeconds);

  PowerMode getPowerMode();
  boolean setPowerMode(PowerMode mode);
  void powerDown();

  uint32_t getUpTime();
}

#endif