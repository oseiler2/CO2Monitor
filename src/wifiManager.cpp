#include "globals.h"
#include <wifiManager.h>
#include <config.h>

#include <ESPAsync_WiFiManager.h>
#include <ESPAsync_WiFiManager-Impl.h>
#include <base64.h>
#include <configManager.h>
#include <lcd.h>

#ifndef AP_PW
#define AP_PW ""
#endif

// Local logging tag
static const char TAG[] = __FILE__;

namespace WifiManager {
#define HTTP_PORT 80

  volatile static boolean safeConfigFlag = false;

  String getMac() {
    uint8_t rawMac[6];
    for (uint8_t i = 0;i < 6;i++) {
      rawMac[i] = (uint8_t)(ESP.getEfuseMac() >> (6 - i - 1) * 8 & 0x000000ffUL);
    }
    String encoded = base64::encode(rawMac, 6);
    encoded.replace("+", "-");
    encoded.replace("/", "_");
    return encoded;
  }

  String getSSID() {
    return ("CO2-Monitor-" + getMac());
  }

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
  ESPAsync_WMParameter* mqttUseTlsParam;
  ESPAsync_WMParameter* mqttInsecureParam;
  ESPAsync_WMParameter* altitudeParam;
  ESPAsync_WMParameter* co2YellowThresholdParam;
  ESPAsync_WMParameter* co2RedThresholdParam;
  ESPAsync_WMParameter* co2DarkRedThresholdParam;
  ESPAsync_WMParameter* iaqYellowThresholdParam;
  ESPAsync_WMParameter* iaqRedThresholdParam;
  ESPAsync_WMParameter* iaqDarkRedThresholdParam;
  ESPAsync_WMParameter* brightnessParam;
  ESPAsync_WMParameter* ssd1306RowsParam;
  ESPAsync_WMParameter* greenLedParam;
  ESPAsync_WMParameter* yellowLedParam;
  ESPAsync_WMParameter* redLedParam;
  ESPAsync_WMParameter* neopixelDataParam;
  ESPAsync_WMParameter* neopixelNumberParam;
  ESPAsync_WMParameter* featherMatrixDataParam;
  ESPAsync_WMParameter* featherMatrixClockParam;
  ESPAsync_WMParameter* hub75R1Param;
  ESPAsync_WMParameter* hub75G1Param;
  ESPAsync_WMParameter* hub75B1Param;
  ESPAsync_WMParameter* hub75R2Param;
  ESPAsync_WMParameter* hub75G2Param;
  ESPAsync_WMParameter* hub75B2Param;
  ESPAsync_WMParameter* hub75ChAParam;
  ESPAsync_WMParameter* hub75ChBParam;
  ESPAsync_WMParameter* hub75ChCParam;
  ESPAsync_WMParameter* hub75ChDParam;
  ESPAsync_WMParameter* hub75ClkParam;
  ESPAsync_WMParameter* hub75LatParam;
  ESPAsync_WMParameter* hub75OeParam;

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
    char mqttUseTls[6];
    char mqttInsecure[6];
    char altitude[5];
    char co2YellowThreshold[5];
    char co2RedThreshold[5];
    char co2DarkRedThreshold[5];
    char iaqYellowThreshold[5];
    char iaqRedThreshold[5];
    char iaqDarkRedThreshold[5];
    char brightness[4];
    char ssd1306Rows[3];
    char greenLed[3];
    char yellowLed[3];
    char redLed[3];
    char neopixelData[3];
    char neopixelNumber[4];
    char featherMatrixData[3];
    char featherMatrixClock[3];
    char hub75R1[3];
    char hub75G1[3];
    char hub75B1[3];
    char hub75R2[3];
    char hub75G2[3];
    char hub75B2[3];
    char hub75ChA[3];
    char hub75ChB[3];
    char hub75ChC[3];
    char hub75ChD[3];
    char hub75Clk[3];
    char hub75Lat[3];
    char hub75Oe[3];

    sprintf(deviceId, "%u", config.deviceId);
    sprintf(mqttTopic, "%s", config.mqttTopic);
    sprintf(mqttUsername, "%s", config.mqttUsername);
    sprintf(mqttPassword, "%s", config.mqttPassword);
    sprintf(mqttHost, "%s", config.mqttHost);
    sprintf(mqttPort, "%u", config.mqttServerPort);
    sprintf(mqttUseTls, "%s", config.mqttUseTls ? "true" : "false");
    sprintf(mqttInsecure, "%s", config.mqttInsecure ? "true" : "false");
    sprintf(altitude, "%u", config.altitude);
    sprintf(co2YellowThreshold, "%u", config.co2YellowThreshold);
    sprintf(co2RedThreshold, "%u", config.co2RedThreshold);
    sprintf(co2DarkRedThreshold, "%u", config.co2DarkRedThreshold);
    sprintf(iaqYellowThreshold, "%u", config.iaqYellowThreshold);
    sprintf(iaqRedThreshold, "%u", config.iaqRedThreshold);
    sprintf(iaqDarkRedThreshold, "%u", config.iaqDarkRedThreshold);
    sprintf(brightness, "%u", config.brightness);
    sprintf(ssd1306Rows, "%u", config.ssd1306Rows);
    sprintf(greenLed, "%u", config.greenLed);
    sprintf(yellowLed, "%u", config.yellowLed);
    sprintf(redLed, "%u", config.redLed);
    sprintf(neopixelData, "%u", config.neopixelData);
    sprintf(neopixelNumber, "%u", config.neopixelNumber);
    sprintf(featherMatrixData, "%u", config.featherMatrixData);
    sprintf(featherMatrixClock, "%u", config.featherMatrixClock);
    sprintf(hub75R1, "%u", config.hub75R1);
    sprintf(hub75G1, "%u", config.hub75G1);
    sprintf(hub75B1, "%u", config.hub75B1);
    sprintf(hub75R2, "%u", config.hub75R2);
    sprintf(hub75G2, "%u", config.hub75G2);
    sprintf(hub75B2, "%u", config.hub75B2);
    sprintf(hub75ChA, "%u", config.hub75ChA);
    sprintf(hub75ChB, "%u", config.hub75ChB);
    sprintf(hub75ChC, "%u", config.hub75ChC);
    sprintf(hub75ChD, "%u", config.hub75ChD);
    sprintf(hub75Clk, "%u", config.hub75Clk);
    sprintf(hub75Lat, "%u", config.hub75Lat);
    sprintf(hub75Oe, "%u", config.hub75Oe);

    ESP_LOGD(TAG, "deviceId: %s", deviceId);
    ESP_LOGD(TAG, "mqttTopic: %s", mqttTopic);
    ESP_LOGD(TAG, "mqttUsername: %s", mqttUsername);
    ESP_LOGD(TAG, "mqttPassword: %s", mqttPassword);
    ESP_LOGD(TAG, "mqttHost: %s", mqttHost);
    ESP_LOGD(TAG, "mqttPort: %s", mqttPort);
    ESP_LOGD(TAG, "mqttUseTls: %s", mqttUseTls);
    ESP_LOGD(TAG, "mqttInsecure: %s", mqttInsecure);
    ESP_LOGD(TAG, "altitude: %s", altitude);
    ESP_LOGD(TAG, "co2YellowThreshold: %s", co2YellowThreshold);
    ESP_LOGD(TAG, "co2RedThreshold: %s", co2RedThreshold);
    ESP_LOGD(TAG, "co2DarkRedThreshold: %s", co2DarkRedThreshold);
    ESP_LOGD(TAG, "iaqYellowThreshold: %s", iaqYellowThreshold);
    ESP_LOGD(TAG, "iaqRedThreshold: %s", iaqRedThreshold);
    ESP_LOGD(TAG, "iaqDarkRedThreshold: %s", iaqDarkRedThreshold);
    ESP_LOGD(TAG, "brightness: %s", brightness);
    ESP_LOGD(TAG, "ssd1306Rows: %s", ssd1306Rows);
    ESP_LOGD(TAG, "greenLed: %s", greenLed);
    ESP_LOGD(TAG, "yellowLed: %s", yellowLed);
    ESP_LOGD(TAG, "redLed: %s", redLed);
    ESP_LOGD(TAG, "neopixelData: %s", neopixelData);
    ESP_LOGD(TAG, "neopixelNumber: %s", neopixelNumber);
    ESP_LOGD(TAG, "featherMatrixData: %s", featherMatrixData);
    ESP_LOGD(TAG, "featherMatrixClock: %s", featherMatrixClock);
    ESP_LOGD(TAG, "hub75R1: %s", hub75R1);
    ESP_LOGD(TAG, "hub75G1: %s", hub75G1);
    ESP_LOGD(TAG, "hub75B1: %s", hub75B1);
    ESP_LOGD(TAG, "hub75R2: %s", hub75R2);
    ESP_LOGD(TAG, "hub75G2: %s", hub75G2);
    ESP_LOGD(TAG, "hub75B2: %s", hub75B2);
    ESP_LOGD(TAG, "hub75ChA: %s", hub75ChA);
    ESP_LOGD(TAG, "hub75ChB: %s", hub75ChB);
    ESP_LOGD(TAG, "hub75ChC: %s", hub75ChC);
    ESP_LOGD(TAG, "hub75ChD: %s", hub75ChD);
    ESP_LOGD(TAG, "hub75Clk: %s", hub75Clk);
    ESP_LOGD(TAG, "hub75Lat: %s", hub75Lat);
    ESP_LOGD(TAG, "hub75Oe: %s", hub75Oe);

    deviceIdParam = new ESPAsync_WMParameter("deviceId", "Device ID", deviceId, 6, "config.deviceId");
    mqttTopicParam = new ESPAsync_WMParameter("mqttTopic", "MQTT topic", mqttTopic, MQTT_TOPIC_ID_LEN + 1, config.mqttTopic);
    mqttUsernameParam = new ESPAsync_WMParameter("mqttUsername", "MQTT username", mqttUsername, MQTT_USERNAME_LEN + 1, config.mqttUsername);
    mqttPasswordParam = new ESPAsync_WMParameter("mqttPassword", "MQTT password", mqttPassword, MQTT_PASSWORD_LEN + 1, config.mqttPassword);
    mqttHostParam = new ESPAsync_WMParameter("mqttHost", "MQTT host", mqttHost, MQTT_HOSTNAME_LEN + 1, config.mqttHost);
    mqttPortParam = new ESPAsync_WMParameter("mqttServerPort", "MQTT port", mqttPort, 6, "config.mqttServerPort");
    mqttUseTlsParam = new ESPAsync_WMParameter("mqttUseTls", "MQTT use TLS", mqttUseTls, 6, "config.mqttUseTls");
    mqttInsecureParam = new ESPAsync_WMParameter("mqttInsecure", "MQTT Ignore certificate errors", mqttInsecure, 6, "config.mqttInsecure");
    altitudeParam = new ESPAsync_WMParameter("altitude", "Altitude", altitude, 5, "config.altitude");
    co2YellowThresholdParam = new ESPAsync_WMParameter("co2YellowThreshold", "CO2 Yellow threshold ", co2YellowThreshold, 6, "config.co2YellowThreshold");
    co2RedThresholdParam = new ESPAsync_WMParameter("co2RedThreshold", "CO2 Red threshold", co2RedThreshold, 6, "config.co2RedThreshold");
    co2DarkRedThresholdParam = new ESPAsync_WMParameter("co2DarkRedThreshold", "CO2 Dark red threshold", co2DarkRedThreshold, 6, "config.co2DarkRedThreshold");
    iaqYellowThresholdParam = new ESPAsync_WMParameter("iaqYellowThreshold", "IAQ Yellow threshold ", iaqYellowThreshold, 6, "config.iaqYellowThreshold");
    iaqRedThresholdParam = new ESPAsync_WMParameter("iaqRedThreshold", "IAQ Red threshold", iaqRedThreshold, 6, "config.iaqRedThreshold");
    iaqDarkRedThresholdParam = new ESPAsync_WMParameter("iaqDarkRedThreshold", "IAQ Dark red threshold", iaqDarkRedThreshold, 6, "config.iaqDarkRedThreshold");
    brightnessParam = new ESPAsync_WMParameter("brightness", "LED brightness pwm", brightness, 4, "config.brightness");
    ssd1306RowsParam = new ESPAsync_WMParameter("ssd1306Rows", "SSD1306 rows", ssd1306Rows, 4, "config.ssd1306Rows");
    greenLedParam = new ESPAsync_WMParameter("greenLed", "Green Led pin", greenLed, 3, "config.greenLed");
    yellowLedParam = new ESPAsync_WMParameter("yellowLed", "Yellow Led pin", yellowLed, 3, "config.yellowLed");
    redLedParam = new ESPAsync_WMParameter("redLed", "Red Led pin", redLed, 3, "config.redLed");
    neopixelDataParam = new ESPAsync_WMParameter("neopixelData", "Neopixel data pin", neopixelData, 3, "config.neopixelData");
    neopixelNumberParam = new ESPAsync_WMParameter("neopixelNumber", "Number of Neopixels", neopixelNumber, 4, "config.neopixelNumber");
    featherMatrixDataParam = new ESPAsync_WMParameter("featherMatrixData", "Feather matrix data pin", featherMatrixData, 3, "config.featherMatrixData");
    featherMatrixClockParam = new ESPAsync_WMParameter("featherMatrixClock", "Feather matrix clock pin", featherMatrixClock, 3, "config.featherMatrixClock");
    hub75R1Param = new ESPAsync_WMParameter("hub75R1", "Hub75 R1 pin", hub75R1, 3, "config.hub75R1");
    hub75G1Param = new ESPAsync_WMParameter("hub75G1", "Hub75 G1 pin", hub75G1, 3, "config.hub75G1");
    hub75B1Param = new ESPAsync_WMParameter("hub75B1", "Hub75 B1 pin", hub75B1, 3, "config.hub75B1");
    hub75R2Param = new ESPAsync_WMParameter("hub75R2", "Hub75 R2 pin", hub75R2, 3, "config.hub75R2");
    hub75G2Param = new ESPAsync_WMParameter("hub75G2", "Hub75 G2 pin", hub75G2, 3, "config.hub75G2");
    hub75B2Param = new ESPAsync_WMParameter("hub75B2", "Hub75 B2 pin", hub75B2, 3, "config.hub75B2");
    hub75ChAParam = new ESPAsync_WMParameter("hub75ChA", "Hub75 Channel A pin", hub75ChA, 3, "config.hub75ChA");
    hub75ChBParam = new ESPAsync_WMParameter("hub75ChB", "Hub75 Channel B pin", hub75ChB, 3, "config.hub75ChB");
    hub75ChCParam = new ESPAsync_WMParameter("hub75ChC", "Hub75 Channel C pin", hub75ChC, 3, "config.hub75ChC");
    hub75ChDParam = new ESPAsync_WMParameter("hub75ChD", "Hub75 Channel D pin", hub75ChD, 3, "config.hub75ChD");
    hub75ClkParam = new ESPAsync_WMParameter("hub75Clk", "Hub75 Clk pin", hub75Clk, 3, "config.hub75Clk");
    hub75LatParam = new ESPAsync_WMParameter("hub75Lat", "Hub75 Lat pin", hub75Lat, 3, "config.hub75Lat");
    hub75OeParam = new ESPAsync_WMParameter("hub75Oe", "Hub75 Oe pin", hub75Oe, 3, "config.hub75Oe");

    wifiManager->addParameter(deviceIdParam);
    wifiManager->addParameter(mqttTopicParam);
    wifiManager->addParameter(mqttUsernameParam);
    wifiManager->addParameter(mqttPasswordParam);
    wifiManager->addParameter(mqttHostParam);
    wifiManager->addParameter(mqttPortParam);
    wifiManager->addParameter(mqttUseTlsParam);
    wifiManager->addParameter(mqttInsecureParam);
    wifiManager->addParameter(altitudeParam);
    wifiManager->addParameter(co2YellowThresholdParam);
    wifiManager->addParameter(co2RedThresholdParam);
    wifiManager->addParameter(co2DarkRedThresholdParam);
    wifiManager->addParameter(iaqYellowThresholdParam);
    wifiManager->addParameter(iaqRedThresholdParam);
    wifiManager->addParameter(iaqDarkRedThresholdParam);
    wifiManager->addParameter(brightnessParam);
    wifiManager->addParameter(ssd1306RowsParam);
    wifiManager->addParameter(greenLedParam);
    wifiManager->addParameter(yellowLedParam);
    wifiManager->addParameter(redLedParam);
    wifiManager->addParameter(neopixelDataParam);
    wifiManager->addParameter(neopixelNumberParam);
    wifiManager->addParameter(featherMatrixDataParam);
    wifiManager->addParameter(featherMatrixClockParam);
    wifiManager->addParameter(hub75R1Param);
    wifiManager->addParameter(hub75G1Param);
    wifiManager->addParameter(hub75B1Param);
    wifiManager->addParameter(hub75R2Param);
    wifiManager->addParameter(hub75G2Param);
    wifiManager->addParameter(hub75B2Param);
    wifiManager->addParameter(hub75ChAParam);
    wifiManager->addParameter(hub75ChBParam);
    wifiManager->addParameter(hub75ChCParam);
    wifiManager->addParameter(hub75ChDParam);
    wifiManager->addParameter(hub75ClkParam);
    wifiManager->addParameter(hub75LatParam);
    wifiManager->addParameter(hub75OeParam);
    wifiManager->setSaveConfigCallback(saveConfigCallback);

    ESP_LOGD(TAG, "SSID: %s", getSSID().c_str());
  }

  void updateConfiguration(ESPAsync_WiFiManager* wifiManager) {
    if (safeConfigFlag) {
      ESP_LOGD(TAG, "deviceId: %s", deviceIdParam->getValue());
      ESP_LOGD(TAG, "mqttTopic: %s", mqttTopicParam->getValue());
      ESP_LOGD(TAG, "mqttUsername: %s", mqttUsernameParam->getValue());
      ESP_LOGD(TAG, "mqttPassword: %s", mqttPasswordParam->getValue());
      ESP_LOGD(TAG, "mqttHost: %s", mqttHostParam->getValue());
      ESP_LOGD(TAG, "mqttPort: %s", mqttPortParam->getValue());
      ESP_LOGD(TAG, "mqttUseTls: %s", mqttUseTlsParam->getValue());
      ESP_LOGD(TAG, "mqttInsecure: %s", mqttInsecureParam->getValue());
      ESP_LOGD(TAG, "altitude: %s", altitudeParam->getValue());
      ESP_LOGD(TAG, "co2YellowThreshold: %s", co2YellowThresholdParam->getValue());
      ESP_LOGD(TAG, "co2RedThreshold: %s", co2RedThresholdParam->getValue());
      ESP_LOGD(TAG, "co2DarkRedThreshold: %s", co2DarkRedThresholdParam->getValue());
      ESP_LOGD(TAG, "iaqYellowThreshold: %s", iaqYellowThresholdParam->getValue());
      ESP_LOGD(TAG, "iaqRedThreshold: %s", iaqRedThresholdParam->getValue());
      ESP_LOGD(TAG, "iaqDarkRedThreshold: %s", iaqDarkRedThresholdParam->getValue());
      ESP_LOGD(TAG, "brightness: %s", brightnessParam->getValue());
      ESP_LOGD(TAG, "ssd1306Rows: %s", ssd1306RowsParam->getValue());
      ESP_LOGD(TAG, "greenLed: %s", greenLedParam->getValue());
      ESP_LOGD(TAG, "yellowLed: %s", yellowLedParam->getValue());
      ESP_LOGD(TAG, "redLed: %s", redLedParam->getValue());
      ESP_LOGD(TAG, "neopixelData: %s", neopixelDataParam->getValue());
      ESP_LOGD(TAG, "neopixelNumber: %s", neopixelNumberParam->getValue());
      ESP_LOGD(TAG, "featherMatrixData: %s", featherMatrixDataParam->getValue());
      ESP_LOGD(TAG, "featherMatrixClock: %s", featherMatrixClockParam->getValue());
      ESP_LOGD(TAG, "hub75R1: %s", hub75R1Param->getValue());
      ESP_LOGD(TAG, "hub75G1: %s", hub75G1Param->getValue());
      ESP_LOGD(TAG, "hub75B1: %s", hub75B1Param->getValue());
      ESP_LOGD(TAG, "hub75R2: %s", hub75R2Param->getValue());
      ESP_LOGD(TAG, "hub75G2: %s", hub75G2Param->getValue());
      ESP_LOGD(TAG, "hub75B2: %s", hub75B2Param->getValue());
      ESP_LOGD(TAG, "hub75ChA: %s", hub75ChAParam->getValue());
      ESP_LOGD(TAG, "hub75ChB: %s", hub75ChBParam->getValue());
      ESP_LOGD(TAG, "hub75ChC: %s", hub75ChCParam->getValue());
      ESP_LOGD(TAG, "hub75ChD: %s", hub75ChDParam->getValue());
      ESP_LOGD(TAG, "hub75Clk: %s", hub75ClkParam->getValue());
      ESP_LOGD(TAG, "hub75Lat: %s", hub75LatParam->getValue());
      ESP_LOGD(TAG, "hub75Oe: %s", hub75OeParam->getValue());
      config.deviceId = (uint16_t)atoi(deviceIdParam->getValue());
      strncpy(config.mqttTopic, mqttTopicParam->getValue(), MQTT_TOPIC_ID_LEN);
      strncpy(config.mqttUsername, mqttUsernameParam->getValue(), MQTT_USERNAME_LEN);
      strncpy(config.mqttPassword, mqttPasswordParam->getValue(), MQTT_PASSWORD_LEN);
      strncpy(config.mqttHost, mqttHostParam->getValue(), MQTT_HOSTNAME_LEN);
      config.mqttServerPort = (uint16_t)atoi(mqttPortParam->getValue());
      config.mqttUseTls = strcmp("true", mqttUseTlsParam->getValue()) == 0;
      config.mqttInsecure = strcmp("true", mqttInsecureParam->getValue()) == 0;
      config.altitude = (uint16_t)atoi(altitudeParam->getValue());
      config.co2YellowThreshold = (uint16_t)atoi(co2YellowThresholdParam->getValue());
      config.co2RedThreshold = (uint16_t)atoi(co2RedThresholdParam->getValue());
      config.co2DarkRedThreshold = (uint16_t)atoi(co2DarkRedThresholdParam->getValue());
      config.iaqYellowThreshold = (uint16_t)atoi(iaqYellowThresholdParam->getValue());
      config.iaqRedThreshold = (uint16_t)atoi(iaqRedThresholdParam->getValue());
      config.iaqDarkRedThreshold = (uint16_t)atoi(iaqDarkRedThresholdParam->getValue());
      config.brightness = (uint8_t)atoi(brightnessParam->getValue());
      config.ssd1306Rows = (uint8_t)atoi(ssd1306RowsParam->getValue());
      config.greenLed = (uint8_t)atoi(greenLedParam->getValue());
      config.yellowLed = (uint8_t)atoi(yellowLedParam->getValue());
      config.redLed = (uint8_t)atoi(redLedParam->getValue());
      config.neopixelData = (uint8_t)atoi(neopixelDataParam->getValue());
      config.neopixelNumber = (uint8_t)atoi(neopixelNumberParam->getValue());
      config.featherMatrixData = (uint8_t)atoi(featherMatrixDataParam->getValue());
      config.featherMatrixClock = (uint8_t)atoi(featherMatrixClockParam->getValue());
      config.hub75R1 = (uint8_t)atoi(hub75R1Param->getValue());
      config.hub75G1 = (uint8_t)atoi(hub75G1Param->getValue());
      config.hub75B1 = (uint8_t)atoi(hub75B1Param->getValue());
      config.hub75R2 = (uint8_t)atoi(hub75R2Param->getValue());
      config.hub75G2 = (uint8_t)atoi(hub75G2Param->getValue());
      config.hub75B2 = (uint8_t)atoi(hub75B2Param->getValue());
      config.hub75ChA = (uint8_t)atoi(hub75ChAParam->getValue());
      config.hub75ChB = (uint8_t)atoi(hub75ChBParam->getValue());
      config.hub75ChC = (uint8_t)atoi(hub75ChCParam->getValue());
      config.hub75ChD = (uint8_t)atoi(hub75ChDParam->getValue());
      config.hub75Clk = (uint8_t)atoi(hub75ClkParam->getValue());
      config.hub75Lat = (uint8_t)atoi(hub75LatParam->getValue());
      config.hub75Oe = (uint8_t)atoi(hub75OeParam->getValue());
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
    delete mqttUseTlsParam;
    delete mqttInsecureParam;
    delete altitudeParam;
    delete co2YellowThresholdParam;
    delete co2RedThresholdParam;
    delete co2DarkRedThresholdParam;
    delete iaqYellowThresholdParam;
    delete iaqRedThresholdParam;
    delete iaqDarkRedThresholdParam;
    delete brightnessParam;
    delete ssd1306RowsParam;
    delete greenLedParam;
    delete yellowLedParam;
    delete redLedParam;
    delete neopixelDataParam;
    delete neopixelNumberParam;
    delete featherMatrixDataParam;
    delete featherMatrixClockParam;
    delete hub75R1Param;
    delete hub75G1Param;
    delete hub75B1Param;
    delete hub75R2Param;
    delete hub75G2Param;
    delete hub75B2Param;
    delete hub75ChAParam;
    delete hub75ChBParam;
    delete hub75ChCParam;
    delete hub75ChDParam;
    delete hub75ClkParam;
    delete hub75LatParam;
    delete hub75OeParam;
  }

  void setupWifi(setPriorityMessageCallback_t setPriorityMessageCallback, clearPriorityMessageCallback_t clearPriorityMessageCallback) {
    if (WiFi.status() != WL_CONNECTED) {
      ESP_LOGD(TAG, "Could not connect to Wifi using known settings");
      AsyncWebServer webServer(HTTP_PORT);
      DNSServer dnsServer;
      ESPAsync_WiFiManager* wifiManager;
      wifiManager = new ESPAsync_WiFiManager(&webServer, &dnsServer, "CO2 Monitor");
      setupWifiManager(wifiManager);
      setPriorityMessageCallback(getSSID().c_str());
      wifiManager->autoConnect(getSSID().c_str(), AP_PW);
      updateConfiguration(wifiManager);
      delete wifiManager;
      clearPriorityMessageCallback();
    }
    ESP_LOGD(TAG, "setupWifi end");
  }

  void startConfigPortal(updateMessageCallback_t updateMessageCallback, setPriorityMessageCallback_t setPriorityMessageCallback, clearPriorityMessageCallback_t clearPriorityMessageCallback) {
    setPriorityMessageCallback(getSSID().c_str());
    AsyncWebServer webServer(HTTP_PORT);
    DNSServer dnsServer;
    ESPAsync_WiFiManager* wifiManager;
    wifiManager = new ESPAsync_WiFiManager(&webServer, &dnsServer, "CO2 Monitor");
    setupWifiManager(wifiManager);
    wifiManager->startConfigPortal(getSSID().c_str(), AP_PW);
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