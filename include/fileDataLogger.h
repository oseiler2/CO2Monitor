#pragma once

#include <globals.h>
#include <model.h>

namespace FileDataLogger {
  boolean writeEvent(const char* mountPoint, int16_t mask, Model* model, TrafficLightStatus status, uint16_t batInMV);
}
