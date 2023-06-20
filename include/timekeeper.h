#ifndef _TIMEKEEPER_H
#define _TIMEKEEPER_H

#include <globals.h>

namespace Timekeeper {
  void init();
  void initSntp();
  void printTime();
  boolean isSynchronised();
}

#endif