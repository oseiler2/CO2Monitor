#include "globals.h"
#include <wifiManager.h>
#include <config.h>

#include <ESPAsync_WiFiManager.h>
#include <ESPAsync_WiFiManager-Impl.h>
#include <ap_pw.h>
#include <configManager.h>
#include <lcd.h>

// Local logging tag
static const char TAG[] = __FILE__;

namespace WifiManager {
#define HTTP_PORT 80

  volatile static boolean safeConfigFlag = false;

  void saveConfigCallback() {
    ESP_LOGD(TAG, "saveConfigCallback");
    safeConfigFlag = true;
  }

  ESPAsync_WMParameter* deviceIdParam;
  ESPAsync_WMParameter* mqttTopicParam;
  ESPAsync_WMParameter* mqttUsernameParam;
  ESPAsync_WMParameter* mqttPasswordParam;
  ESPAsync_WMParameter* mqttHostParam;
  ESPAsync_WMParameter* mqttPortParam;
  ESPAsync_WMParameter* altitudeParam;
  ESPAsync_WMParameter* yellowThresholdParam;
  ESPAsync_WMParameter* redThresholdParam;
  ESPAsync_WMParameter* darkRedThresholdParam;
  ESPAsync_WMParameter* ledPwmParam;

  void setupWifiManager(ESPAsync_WiFiManager* wifiManager) {
    safeConfigFlag = false;
    wifiManager->setConfigPortalTimeout(300);

    WiFi_AP_IPConfig  portalIPconfig;
    portalIPconfig._ap_static_gw = IPAddress(192, 168, 100, 1);
    portalIPconfig._ap_static_ip = IPAddress(192, 168, 100, 1);
    portalIPconfig._ap_static_sn = IPAddress(255, 255, 255, 0);
    wifiManager->setAPStaticIPConfig(portalIPconfig);

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
    char ledPwm[4];

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
    sprintf(ledPwm, "%u", config.ledPwm);

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
    ESP_LOGD(TAG, "ledPwm: %s", ledPwm);

    deviceIdParam = new ESPAsync_WMParameter("deviceId", "Device ID", deviceId, 5, "config.deviceId");
    mqttTopicParam = new ESPAsync_WMParameter("mqttTopic", "MQTT topic ", mqttTopic, MQTT_TOPIC_ID_LEN, config.mqttTopic);
    mqttUsernameParam = new ESPAsync_WMParameter("mqttUsername", "MQTT username ", mqttUsername, MQTT_USERNAME_LEN, config.mqttUsername);
    mqttPasswordParam = new ESPAsync_WMParameter("mqttPassword", "MQTT password ", mqttPassword, MQTT_PASSWORD_LEN, config.mqttPassword);
    mqttHostParam = new ESPAsync_WMParameter("mqttHost", "MQTT host ", mqttHost, MQTT_HOSTNAME_LEN, config.mqttHost);
    mqttPortParam = new ESPAsync_WMParameter("mqttServerPort", "MQTT port ", mqttPort, 5, "config.mqttServerPort");
    altitudeParam = new ESPAsync_WMParameter("altitude", "Altitude ", altitude, 4, "config.altitude");
    yellowThresholdParam = new ESPAsync_WMParameter("yellowThreshold", "Yellow threshold ", yellowThreshold, 5, "config.yellowThreshold");
    redThresholdParam = new ESPAsync_WMParameter("redThreshold", "Red threshold ", redThreshold, 5, "config.redThreshold");
    darkRedThresholdParam = new ESPAsync_WMParameter("darkRedThreshold", "Dark red threshold ", darkRedThreshold, 5, "config.darkRedThreshold");
    ledPwmParam = new ESPAsync_WMParameter("ledPwm", "LED brightness pwm ", ledPwm, 4, "config.ledPwm");

    wifiManager->addParameter(deviceIdParam);
    wifiManager->addParameter(mqttTopicParam);
    wifiManager->addParameter(mqttUsernameParam);
    wifiManager->addParameter(mqttPasswordParam);
    wifiManager->addParameter(mqttHostParam);
    wifiManager->addParameter(mqttPortParam);
    wifiManager->addParameter(altitudeParam);
    wifiManager->addParameter(yellowThresholdParam);
    wifiManager->addParameter(redThresholdParam);
    wifiManager->addParameter(darkRedThresholdParam);
    wifiManager->addParameter(ledPwmParam);
    wifiManager->setSaveConfigCallback(saveConfigCallback);

    ESP_LOGD(TAG, "SSID: %s", ("CO2-Monitor-" + String((uint32_t)ESP.getEfuseMac(), HEX)).c_str());
  }

  void updateConfiguration(ESPAsync_WiFiManager* wifiManager) {
    if (safeConfigFlag) {
      ESP_LOGD(TAG, "deviceId: %s", deviceIdParam->getValue());
      ESP_LOGD(TAG, "mqttTopic: %s", mqttTopicParam->getValue());
      ESP_LOGD(TAG, "mqttUsername: %s", mqttUsernameParam->getValue());
      ESP_LOGD(TAG, "mqttPassword: %s", mqttPasswordParam->getValue());
      ESP_LOGD(TAG, "mqttHost: %s", mqttHostParam->getValue());
      ESP_LOGD(TAG, "mqttPort: %s", mqttPortParam->getValue());
      ESP_LOGD(TAG, "altitude: %s", altitudeParam->getValue());
      ESP_LOGD(TAG, "yellowThreshold: %s", yellowThresholdParam->getValue());
      ESP_LOGD(TAG, "redThreshold: %s", redThresholdParam->getValue());
      ESP_LOGD(TAG, "darkRedThreshold: %s", darkRedThresholdParam->getValue());
      ESP_LOGD(TAG, "ledPwm: %s", ledPwmParam->getValue());
      config.deviceId = (uint16_t)atoi(deviceIdParam->getValue());
      strncpy(config.mqttTopic, mqttTopicParam->getValue(), MQTT_TOPIC_ID_LEN + 1);
      strncpy(config.mqttUsername, mqttUsernameParam->getValue(), MQTT_USERNAME_LEN + 1);
      strncpy(config.mqttPassword, mqttPasswordParam->getValue(), MQTT_PASSWORD_LEN + 1);
      strncpy(config.mqttHost, mqttHostParam->getValue(), MQTT_HOSTNAME_LEN + 1);
      config.mqttServerPort = (uint16_t)atoi(mqttPortParam->getValue());
      config.altitude = (uint16_t)atoi(altitudeParam->getValue());
      config.yellowThreshold = (uint16_t)atoi(yellowThresholdParam->getValue());
      config.redThreshold = (uint16_t)atoi(redThresholdParam->getValue());
      config.darkRedThreshold = (uint16_t)atoi(darkRedThresholdParam->getValue());
      config.ledPwm = (uint8_t)atoi(ledPwmParam->getValue());
      saveConfiguration(config);
      delay(1000);
      esp_restart();
    }
    delete deviceIdParam;
    delete mqttTopicParam;
    delete mqttUsernameParam;
    delete mqttPasswordParam;
    delete mqttHostParam;
    delete mqttPortParam;
    delete altitudeParam;
    delete yellowThresholdParam;
    delete redThresholdParam;
    delete darkRedThresholdParam;
    delete ledPwmParam;
  }

  void setupWifi(setPriorityMessageCallback_t setPriorityMessageCallback, clearPriorityMessageCallback_t clearPriorityMessageCallback) {
    AsyncWebServer webServer(HTTP_PORT);
    DNSServer dnsServer;
    ESPAsync_WiFiManager* wifiManager;
    wifiManager = new ESPAsync_WiFiManager(&webServer, &dnsServer, "CO2 Monitor");
    setupWifiManager(wifiManager);
    setPriorityMessageCallback(("CO2-Monitor-" + String((uint32_t)ESP.getEfuseMac(), HEX)).c_str());
    wifiManager->autoConnect(("CO2-Monitor-" + String((uint32_t)ESP.getEfuseMac(), HEX)).c_str(), AP_PW);
    updateConfiguration(wifiManager);
    delete wifiManager;
    clearPriorityMessageCallback();
    ESP_LOGD(TAG, "setupWifi end");
  }

  void startConfigPortal(updateMessageCallback_t updateMessageCallback, setPriorityMessageCallback_t setPriorityMessageCallback, clearPriorityMessageCallback_t clearPriorityMessageCallback) {
    setPriorityMessageCallback(("CO2-Monitor-" + String((uint32_t)ESP.getEfuseMac(), HEX)).c_str());
    AsyncWebServer webServer(HTTP_PORT);
    DNSServer dnsServer;
    ESPAsync_WiFiManager* wifiManager;
    wifiManager = new ESPAsync_WiFiManager(&webServer, &dnsServer, "CO2 Monitor");
    setupWifiManager(wifiManager);
    wifiManager->startConfigPortal(("CO2-Monitor-" + String((uint32_t)ESP.getEfuseMac(), HEX)).c_str(), AP_PW);
    updateConfiguration(wifiManager);
    delete wifiManager;
    clearPriorityMessageCallback();
    ESP_LOGD(TAG, "startConfigPortal end");
  }

  void resetSettings() {
    //    ESPAsync_WiFiManager::resetSettings();
    WiFi.disconnect(true, true);
    WiFi.begin("0", "0");
  }

}