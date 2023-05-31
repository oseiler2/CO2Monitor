#include <globals.h>
#include <wifiManager.h>
#include <config.h>

#include <ESPAsync_WiFiManager.h>
#include <ESPAsync_WiFiManager-Impl.h>
#include <base64.h>
#include <configManager.h>

#include <configParameter.h>

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

  template <typename T>
  std::pair<ConfigParameterBase*, ESPAsync_WMParameter*>* toParameterPair(ConfigParameter<T>* configParameter) {
    char defaultValue[configParameter->getMaxStrLen()];
    configParameter->print(defaultValue);
    ESPAsync_WMParameter* espAsyncParam = new ESPAsync_WMParameter(configParameter->getId(), configParameter->getLabel(), defaultValue, configParameter->getMaxStrLen());
    ESP_LOGD(TAG, "%s: %s", configParameter->getId(), defaultValue);
    return new std::pair<ConfigParameterBase*, ESPAsync_WMParameter*>(configParameter, espAsyncParam);
  }

  std::vector<std::pair<ConfigParameterBase*, ESPAsync_WMParameter*>*> configParameterVector;

  void setupWifiManager(ESPAsync_WiFiManager* wifiManager) {
    safeConfigFlag = false;
    wifiManager->setConfigPortalTimeout(300);

    WiFi_AP_IPConfig  portalIPconfig;
    portalIPconfig._ap_static_gw = IPAddress(192, 168, 100, 1);
    portalIPconfig._ap_static_ip = IPAddress(192, 168, 100, 1);
    portalIPconfig._ap_static_sn = IPAddress(255, 255, 255, 0);
    wifiManager->setAPStaticIPConfig(portalIPconfig);

    configParameterVector.push_back(toParameterPair(new Uint16ConfigParameter("deviceId", "Device ID", &(config.*deviceIdPtr))));
    configParameterVector.push_back(toParameterPair(new CharArrayConfigParameter("mqttTopic", "MQTT topic", &(config.*mqttTopicPtr), MQTT_TOPIC_ID_LEN + 1)));
    configParameterVector.push_back(toParameterPair(new CharArrayConfigParameter("mqttUsername", "MQTT username", &(config.*mqttUsernamePtr), MQTT_USERNAME_LEN + 1)));
    configParameterVector.push_back(toParameterPair(new CharArrayConfigParameter("mqttPassword", "MQTT password", &(config.*mqttPasswordPtr), MQTT_PASSWORD_LEN + 1)));
    configParameterVector.push_back(toParameterPair(new CharArrayConfigParameter("mqttHost", "MQTT host", &(config.*mqttHostPtr), MQTT_HOSTNAME_LEN + 1)));
    configParameterVector.push_back(toParameterPair(new Uint16ConfigParameter("mqttServerPort", "MQTT port", &(config.*mqttServerPortPtr))));
    configParameterVector.push_back(toParameterPair(new BooleanConfigParameter("mqttUseTls", "MQTT use TLS", &(config.*mqttUseTlsPtr))));
    configParameterVector.push_back(toParameterPair(new BooleanConfigParameter("mqttInsecure", "MQTT Ignore certificate errors", &(config.*mqttInsecurePtr))));
    configParameterVector.push_back(toParameterPair(new Uint16ConfigParameter("altitude", "Altitude", &(config.*altitudePtr))));
    configParameterVector.push_back(toParameterPair(new Uint16ConfigParameter("co2GreenThreshold", "CO2 Green threshold ", &(config.*co2GreenThresholdPtr))));
    configParameterVector.push_back(toParameterPair(new Uint16ConfigParameter("co2YellowThreshold", "CO2 Yellow threshold ", &(config.*co2YellowThresholdPtr))));
    configParameterVector.push_back(toParameterPair(new Uint16ConfigParameter("co2RedThreshold", "CO2 Red threshold", &(config.*co2RedThresholdPtr))));
    configParameterVector.push_back(toParameterPair(new Uint16ConfigParameter("co2DarkRedThreshold", "CO2 Dark red threshold", &(config.*co2DarkRedThresholdPtr))));
    configParameterVector.push_back(toParameterPair(new Uint16ConfigParameter("iaqGreenThreshold", "IAQ Green threshold ", &(config.*iaqGreenThresholdPtr))));
    configParameterVector.push_back(toParameterPair(new Uint16ConfigParameter("iaqYellowThreshold", "IAQ Yellow threshold ", &(config.*iaqYellowThresholdPtr))));
    configParameterVector.push_back(toParameterPair(new Uint16ConfigParameter("iaqRedThreshold", "IAQ Red threshold", &(config.*iaqRedThresholdPtr))));
    configParameterVector.push_back(toParameterPair(new Uint16ConfigParameter("iaqDarkRedThreshold", "IAQ Dark red threshold", &(config.*iaqDarkRedThresholdPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("brightness", "LED brightness pwm", &(config.*brightnessPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("ssd1306Rows", "SSD1306 rows", &(config.*ssd1306RowsPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("greenLed", "Green Led pin", &(config.*greenLedPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("yellowLed", "Yellow Led pin", &(config.*yellowLedPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("redLed", "Red Led pin", &(config.*redLedPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("neopixelData", "Neopixel data pin", &(config.*neopixelDataPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("neopixelNumber", "Number of Neopixels", &(config.*neopixelNumberPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("neopixelMatrixData", "Neopixel matrix data pin", &(config.*neopixelMatrixDataPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("featherMatrixData", "Feather matrix data pin", &(config.*featherMatrixDataPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("featherMatrixClock", "Feather matrix clock pin", &(config.*featherMatrixClockPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("matrixColumns", "Matrix columns", &(config.*matrixColumnsPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("matrixRows", "Matrix rows", &(config.*matrixRowsPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("matrixLayout", "Matrix layout", &(config.*matrixLayoutPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("hub75R1", "Hub75 R1 pin", &(config.*hub75R1Ptr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("hub75G1", "Hub75 G1 pin", &(config.*hub75G1Ptr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("hub75B1", "Hub75 B1 pin", &(config.*hub75B1Ptr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("hub75R2", "Hub75 R2 pin", &(config.*hub75R2Ptr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("hub75G2", "Hub75 G2 pin", &(config.*hub75G2Ptr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("hub75B2", "Hub75 B2 pin", &(config.*hub75B2Ptr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("hub75ChA", "Hub75 Channel A pin", &(config.*hub75ChAPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("hub75ChB", "Hub75 Channel B pin", &(config.*hub75ChBPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("hub75ChC", "Hub75 Channel C pin", &(config.*hub75ChCPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("hub75ChD", "Hub75 Channel D pin", &(config.*hub75ChDPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("hub75Clk", "Hub75 Clk pin", &(config.*hub75ClkPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("hub75Lat", "Hub75 Lat pin", &(config.*hub75LatPtr))));
    configParameterVector.push_back(toParameterPair(new Uint8ConfigParameter("hub75Oe", "Hub75 Oe pin", &(config.*hub75OePtr))));

    for (std::pair<ConfigParameterBase*, ESPAsync_WMParameter*>* configParameterPair : configParameterVector) {
      wifiManager->addParameter(configParameterPair->second);
    }
    wifiManager->setSaveConfigCallback(saveConfigCallback);

    ESP_LOGD(TAG, "SSID: %s", getSSID().c_str());
  }

  void updateConfiguration(ESPAsync_WiFiManager* wifiManager) {
    if (safeConfigFlag) {
      char msg[128];
      for (std::pair<ConfigParameterBase*, ESPAsync_WMParameter*>* configParameterPair : configParameterVector) {
        configParameterPair->first->print(msg);
        ESP_LOGD(TAG, "%s: %s", configParameterPair->first->getId(), msg);

        configParameterPair->first->save(configParameterPair->second->getValue());
      }
      saveConfiguration(config);
      delay(1000);
      esp_restart();
    }
    for (std::pair<ConfigParameterBase*, ESPAsync_WMParameter*>* configParameterPair : configParameterVector) {
      free(configParameterPair->first);
      free(configParameterPair->second);
      free(configParameterPair);
    }
    configParameterVector.clear();
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