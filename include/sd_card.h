#ifndef _SD_CARD_H
#define _SD_CARD_H

#include <globals.h>
#include <model.h>

namespace SdCard {
  #define MOUNT_POINT "/sdcard"
  boolean probe();
  boolean setup();
  boolean isInitialised();
  boolean writeEvent(int16_t mask, Model* model, TrafficLightStatus status, uint16_t batInMV);
  boolean unmount();
}

#endif