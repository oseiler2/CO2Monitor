#pragma once

#include <globals.h>

namespace Timekeeper {
  void init();
  void initSntp();
  void printTime();
  boolean isSynchronised();
}
