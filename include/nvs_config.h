#ifndef _NVS_H
#define _NVS_H

#include <Arduino.h>

namespace NVS {
  boolean init();
  boolean isInitialised();
  void close();
}


#endif