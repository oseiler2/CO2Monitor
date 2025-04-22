#pragma once

#include <globals.h>
#include <model.h>

namespace SdCard {
  #define SD_MOUNT_POINT "/sdcard"
  boolean probe();
  boolean setup();
  boolean isInitialised();
  boolean writeEvent(int16_t mask, Model* model, TrafficLightStatus status, uint16_t batInMV);
  boolean unmount();
}
