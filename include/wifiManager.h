#ifndef _WIFI_H
#define _WIFI_H

#include <config.h>
#include <ESPAsyncWebServer.h>

namespace WifiManager {
  extern AsyncWebServer webServer;

  void setupWifi();
  void startConfigPortal(updateMessageCallback_t updateMessageCallback);
}

#endif