#ifndef _WIFI_H
#define _WIFI_H

#include <config.h>
#include <ESPAsyncWebServer.h>

extern AsyncWebServer webServer;

void setupWifi();
void startConfigPortal(updateMessageCallback_t updateMessageCallback);

#endif