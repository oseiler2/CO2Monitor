#pragma once

#include <globals.h>
#include <config.h>
#include <callbacks.h>
#include <ESPAsyncWebServer.h>
#include <configParameter.h>
#include <model.h>

namespace WifiManager {
  extern TaskHandle_t wifiManagerTask;

  typedef void (*configChangedCallback_t)();

  void setupWifiManager(const char* appName, std::vector<ConfigParameterBase<Config>*> configParameterVector, bool keepCaptivePortalActive, bool captivePortalActiveWhenNotConnected,
    updateMessageCallback_t updateMessageCallback, setPriorityMessageCallback_t setPriorityMessageCallback, clearPriorityMessageCallback_t clearPriorityMessageCallback,
    configChangedCallback_t configChangedCallback, calibrateCo2SensorCallback_t _calibrateCo2SensorCallback);
  void resetSettings();
  void startCaptivePortal();
  String getMac();
  void wifiManagerLoop(void* pvParameters);
  TaskHandle_t start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core);
  void eventHandler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
}
