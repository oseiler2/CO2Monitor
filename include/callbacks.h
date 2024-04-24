#pragma once

#include <globals.h>

typedef boolean (*calibrateCo2SensorCallback_t)(uint16_t);
typedef void (*setTemperatureOffsetCallback_t)(float);
typedef float (*getTemperatureOffsetCallback_t)(void);
typedef uint32_t(*getSPS30AutoCleanIntervalCallback_t)(void);
typedef boolean(*setSPS30AutoCleanIntervalCallback_t)(uint32_t);
typedef boolean(*cleanSPS30Callback_t)(void);
typedef uint8_t(*getSPS30StatusCallback_t)(void);

typedef void (*configChangedCallback_t)();

typedef void (*updateMessageCallback_t)(char const*);
typedef void (*setPriorityMessageCallback_t)(char const*);
typedef void (*clearPriorityMessageCallback_t)(void);
typedef void (*publishMessageCallback_t)(char const*);

