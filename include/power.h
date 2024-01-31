#ifndef _POWER_H
#define _POWER_H

#include <globals.h>
#include <model.h>

typedef enum {
  RM_UNDEFINED = 0,
  RM_LOW,
  RM_FULL
} RunMode;

typedef enum {
  RR_UNDEFINED = 0,
  FULL_RESET,
  WAKE_FROM_SLEEPTIMER,
  WAKE_FROM_BUTTON
} ResetReason;

extern Model* model;

namespace Power {
  ResetReason afterReset();
  void deepSleep(uint64_t durationInSeconds);

  RunMode getRunMode();
  boolean setRunMode(RunMode mode);

  void powerDown();

  uint32_t getUpTime();
}

#endif