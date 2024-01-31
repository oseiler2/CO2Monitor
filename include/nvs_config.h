#ifndef _NVS_H
#define _NVS_H

#include <Arduino.h>
#include <power.h>

namespace NVS {
  boolean init();
  boolean isInitialised();
  void close();

  RunMode readRunmode();
  boolean writeRunmode(RunMode rm);

}


#endif