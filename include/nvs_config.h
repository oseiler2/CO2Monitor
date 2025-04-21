#pragma once

#include <Arduino.h>
#include <power.h>

namespace NVS {
  boolean init();
  boolean isInitialised();
  void close();

  RunMode readRunmode();
  boolean writeRunmode(RunMode rm);

}
