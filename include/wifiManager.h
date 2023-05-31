#ifndef _WIFI_H
#define _WIFI_H

#include <globals.h>
#include <config.h>
#include <messageSupport.h>
#include <ESPAsyncWebServer.h>

namespace WifiManager {
  extern AsyncWebServer webServer;

  void setupWifi(setPriorityMessageCallback_t setPriorityMessageCallback, clearPriorityMessageCallback_t clearPriorityMessageCallback);
  void startConfigPortal(updateMessageCallback_t updateMessageCallback, setPriorityMessageCallback_t setPriorityMessageCallback, clearPriorityMessageCallback_t clearPriorityMessageCallback);
  void resetSettings();
  String getMac();
}

#endif