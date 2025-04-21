#pragma once

#include <globals.h>
#include <config.h>
#include <configParameter.h>
#include <vector>

extern Config config;

void setupConfigManager();
void getDefaultConfiguration(Config& config);
boolean loadConfiguration(Config& config);
boolean saveConfiguration(const Config config);
void logConfiguration(const Config config);
void printFile();
BuzzerMode getBuzzerModeFromUint(uint8_t buzzerMode);
SleepModeOledLed getSleepModeOledLedFromUint(uint8_t mode);

std::vector<ConfigParameterBase<Config>*> getConfigParameters();
