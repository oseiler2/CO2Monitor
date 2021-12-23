#include "globals.h"
#include <wifiManager.h>
#include <config.h>

#include <ESPAsync_WiFiManager.h>
#include <configManager.h>
#include <lcd.h>

#define HTTP_PORT 80

volatile static boolean safeConfigFlag = false;
void saveConfigCallback() {
  ESP_LOGD(TAG, "saveConfigCallback");
  //  strcpy(config.mqttTopic, mqttTopicParam.getValue());
  //saveConfiguration(config);
  safeConfigFlag = true;
}

void setupWifi() {
  AsyncWebServer webServer(HTTP_PORT);

  DNSServer dnsServer;

  ESPAsync_WiFiManager ESPAsync_wifiManager(&webServer, &dnsServer, "CO2 Monitor");

  WiFi_AP_IPConfig  portalIPconfig;
  portalIPconfig._ap_static_gw = IPAddress(192, 168, 100, 1);
  portalIPconfig._ap_static_ip = IPAddress(192, 168, 100, 1);
  portalIPconfig._ap_static_sn = IPAddress(255, 255, 255, 0);
  ESPAsync_wifiManager.setAPStaticIPConfig(portalIPconfig);

  ESP_LOGD(TAG, "SSID: %s", ("CO2-Monitor-" + String((uint32_t)ESP.getEfuseMac(), HEX)).c_str());
  ESPAsync_wifiManager.autoConnect(("CO2-Monitor-" + String((uint32_t)ESP.getEfuseMac(), HEX)).c_str());
}

void startConfigPortal(updateMessageCallback_t updateMessageCallback) {
  updateMessageCallback(String((uint32_t)ESP.getEfuseMac(), HEX).c_str());
  safeConfigFlag = false;
  AsyncWebServer webServer(HTTP_PORT);

  DNSServer dnsServer;

  ESPAsync_WiFiManager ESPAsync_wifiManager(&webServer, &dnsServer, "CO2 Monitor");

  WiFi_AP_IPConfig  portalIPconfig;
  portalIPconfig._ap_static_gw = IPAddress(192, 168, 100, 1);
  portalIPconfig._ap_static_ip = IPAddress(192, 168, 100, 1);
  portalIPconfig._ap_static_sn = IPAddress(255, 255, 255, 0);
  ESPAsync_wifiManager.setAPStaticIPConfig(portalIPconfig);

  //  Router_SSID = ESPAsync_wifiManager.WiFi_SSID();
  //  Router_Pass = ESPAsync_wifiManager.WiFi_Pass();

  char deviceId[6];
  char mqttTopic[MQTT_TOPIC_ID_LEN + 1];
  char mqttUsername[MQTT_USERNAME_LEN + 1];
  char mqttPassword[MQTT_PASSWORD_LEN + 1];
  char mqttHost[MQTT_HOSTNAME_LEN + 1];
  char mqttPort[6];
  char altitude[5];
  char yellowThreshold[5];
  char redThreshold[5];
  char darkRedThreshold[5];

  sprintf(deviceId, "%u", config.deviceId);
  sprintf(mqttTopic, "%s", config.mqttTopic);
  sprintf(mqttUsername, "%s", config.mqttUsername);
  sprintf(mqttPassword, "%s", config.mqttPassword);
  sprintf(mqttHost, "%s", config.mqttHost);
  sprintf(mqttPort, "%u", config.mqttServerPort);
  sprintf(altitude, "%u", config.altitude);
  sprintf(yellowThreshold, "%u", config.yellowThreshold);
  sprintf(redThreshold, "%u", config.redThreshold);
  sprintf(darkRedThreshold, "%u", config.darkRedThreshold);

  ESP_LOGD(TAG, "deviceId: %s", deviceId);
  ESP_LOGD(TAG, "mqttTopic: %s", mqttTopic);
  ESP_LOGD(TAG, "mqttUsername: %s", mqttUsername);
  ESP_LOGD(TAG, "mqttPassword: %s", mqttPassword);
  ESP_LOGD(TAG, "mqttHost: %s", mqttHost);
  ESP_LOGD(TAG, "mqttPort: %s", mqttPort);
  ESP_LOGD(TAG, "altitude: %s", altitude);
  ESP_LOGD(TAG, "yellowThreshold: %s", yellowThreshold);
  ESP_LOGD(TAG, "redThreshold: %s", redThreshold);
  ESP_LOGD(TAG, "darkRedThreshold: %s", darkRedThreshold);

  ESPAsync_WMParameter deviceIdParam("deviceId", "Device ID", deviceId, 5, "config.deviceId");
  ESPAsync_WMParameter mqttTopicParam("mqttTopic", "MQTT topic ", mqttTopic, MQTT_TOPIC_ID_LEN, config.mqttTopic);
  ESPAsync_WMParameter mqttUsernameParam("mqttUsername", "MQTT username ", mqttUsername, MQTT_USERNAME_LEN, config.mqttUsername);
  ESPAsync_WMParameter mqttPasswordParam("mqttPassword", "MQTT password ", mqttPassword, MQTT_PASSWORD_LEN, config.mqttPassword);
  ESPAsync_WMParameter mqttHostParam("mqttHost", "MQTT host ", mqttHost, MQTT_HOSTNAME_LEN, config.mqttHost);
  ESPAsync_WMParameter mqttPortParam("mqttServerPort", "MQTT port ", mqttPort, 5, "config.mqttServerPort");
  ESPAsync_WMParameter altitudeParam("altitude", "Altitude ", altitude, 4, "config.altitude");
  ESPAsync_WMParameter yellowThresholdParam("yellowThreshold", "Yellow threshold ", yellowThreshold, 5, "config.yellowThreshold");
  ESPAsync_WMParameter redThresholdParam("redThreshold", "Red threshold ", redThreshold, 5, "config.redThreshold");
  ESPAsync_WMParameter darkRedThresholdParam("darkRedThreshold", "Dark red threshold ", darkRedThreshold, 5, "config.darkRedThreshold");

  ESPAsync_wifiManager.addParameter(&deviceIdParam);
  ESPAsync_wifiManager.addParameter(&mqttTopicParam);
  ESPAsync_wifiManager.addParameter(&mqttUsernameParam);
  ESPAsync_wifiManager.addParameter(&mqttPasswordParam);
  ESPAsync_wifiManager.addParameter(&mqttHostParam);
  ESPAsync_wifiManager.addParameter(&mqttPortParam);
  ESPAsync_wifiManager.addParameter(&altitudeParam);
  ESPAsync_wifiManager.addParameter(&yellowThresholdParam);
  ESPAsync_wifiManager.addParameter(&redThresholdParam);
  ESPAsync_wifiManager.addParameter(&darkRedThresholdParam);
  ESPAsync_wifiManager.setSaveConfigCallback(saveConfigCallback);

  ESP_LOGD(TAG, "SSID: %s", ("CO2-Monitor-" + String((uint32_t)ESP.getEfuseMac(), HEX)).c_str());
  ESPAsync_wifiManager.startConfigPortal(("CO2-Monitor-" + String((uint32_t)ESP.getEfuseMac(), HEX)).c_str(), "");

  ESP_LOGD(TAG, "deviceId: %s", deviceIdParam.getValue());
  ESP_LOGD(TAG, "mqttTopic: %s", mqttTopicParam.getValue());
  ESP_LOGD(TAG, "mqttUsername: %s", mqttUsernameParam.getValue());
  ESP_LOGD(TAG, "mqttPassword: %s", mqttPasswordParam.getValue());
  ESP_LOGD(TAG, "mqttHost: %s", mqttHostParam.getValue());
  ESP_LOGD(TAG, "mqttPort: %s", mqttPortParam.getValue());
  ESP_LOGD(TAG, "altitude: %s", altitudeParam.getValue());
  ESP_LOGD(TAG, "yellowThreshold: %s", yellowThresholdParam.getValue());
  ESP_LOGD(TAG, "redThreshold: %s", redThresholdParam.getValue());
  ESP_LOGD(TAG, "darkRedThreshold: %s", darkRedThresholdParam.getValue());

  ESP_LOGD(TAG, "safeConfigFlag: %u", safeConfigFlag);

  if (safeConfigFlag) {
    config.deviceId = (uint16_t)atoi(deviceIdParam.getValue());
    strncpy(config.mqttTopic, mqttTopicParam.getValue(), MQTT_TOPIC_ID_LEN);
    strncpy(config.mqttUsername, mqttUsernameParam.getValue(), MQTT_USERNAME_LEN);
    strncpy(config.mqttPassword, mqttPasswordParam.getValue(), MQTT_PASSWORD_LEN);
    strncpy(config.mqttHost, mqttHostParam.getValue(), MQTT_HOSTNAME_LEN);
    config.mqttServerPort = (uint16_t)atoi(mqttPortParam.getValue());
    config.altitude = (uint16_t)atoi(altitudeParam.getValue());
    config.yellowThreshold = (uint16_t)atoi(yellowThresholdParam.getValue());
    config.redThreshold = (uint16_t)atoi(redThresholdParam.getValue());
    config.darkRedThreshold = (uint16_t)atoi(darkRedThresholdParam.getValue());
    saveConfiguration(config);
    delay(1000);
    esp_restart();
  }
  ESP_LOGD(TAG, "startConfigPortal end");
}

