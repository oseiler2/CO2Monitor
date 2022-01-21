#ifndef _OTA_H
#define _OTA_H

#include "globals.h"

namespace OTA {
  void setupOta();
  void checkForUpdate();
  extern TaskHandle_t otaTask;
  void otaLoop(void* pvParameters);
}

#endif