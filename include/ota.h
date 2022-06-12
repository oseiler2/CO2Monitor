#ifndef _OTA_H
#define _OTA_H

#include "globals.h"


typedef void (*preUpdateCallback_t)(void);

namespace OTA {
  void setupOta(preUpdateCallback_t preUpdateCallback);
  void checkForUpdate();
  extern TaskHandle_t otaTask;
  void otaLoop(void* pvParameters);
  void forceUpdate(char* url);
}

#endif