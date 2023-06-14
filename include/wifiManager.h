#ifndef _WIFI_H
#define _WIFI_H

#include <globals.h>
#include <config.h>
#include <messageSupport.h>
#include <ESPAsyncWebServer.h>
#include <configParameter.h>
#include <model.h>

typedef void (*configChangedCallback_t)();

namespace WifiManager {
  extern TaskHandle_t wifiManagerTask;

  void setupWifiManager(const char* appName, std::vector<ConfigParameterBase<Config>*> configParameterVector, bool keepCaptivePortalActive, bool captivePortalActiveWhenNotConnected,
    updateMessageCallback_t updateMessageCallback, setPriorityMessageCallback_t setPriorityMessageCallback, clearPriorityMessageCallback_t clearPriorityMessageCallback,
    configChangedCallback_t configChangedCallback);
  void resetSettings();
  void startCaptivePortal();
  String getMac();
  void wifiManagerLoop(void* pvParameters);
  TaskHandle_t start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core);
  void eventHandler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
}

#endif