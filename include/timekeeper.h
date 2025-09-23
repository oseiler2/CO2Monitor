#pragma once

#include <globals.h>

namespace Timekeeper {
  void init(boolean wakeFromDeepSleep);
  void initSntp();
  void printTime();
  boolean isSynchronised();
}
