#include <globals.h>
#include <logging.h>

#include <wifiManager.h>
#include <html.h>
#include <config.h>
#include <configManager.h>

#include <base64.h>
#include <esp_wifi.h>
#include <DNSServer.h>

#include <ImprovWiFiLibrary.h>

#include <timekeeper.h>

#ifndef AP_PW
#define AP_PW ""
#endif

#ifndef PORTAL_USER
#define PORTAL_USER ""
#endif

#ifndef PORTAL_PW
#define PORTAL_PW ""
#endif

// Local logging tag
static const char TAG[] = __FILE__;

namespace WifiManager {

#define HTTP_PORT 80
#define MAX_SSID_LEN 32
#define MAX_PW_LEN 64

  char ssid[MAX_SSID_LEN + 1];
  char password[MAX_PW_LEN + 1];

  TaskHandle_t wifiManagerTask;

  const uint8_t X_CMD_CONNECT = bit(0);
  const uint8_t X_CMD_SAVE_CONFIG = bit(1);
  const uint8_t X_CMD_SAVE_CONFIG_AND_REBOOT = bit(2);
  const uint8_t X_CMD_WIFI_SCAN_DONE = bit(3);

  bool keepCaptivePortalActive;
  bool captivePortalActiveWhenNotConnected;

  const uint32_t WIFI_SCAN_REUSE = 30 * 000;          // if a Wifi scan was done more recent that this it'll be used, otherwise a new scan triggered
  const uint32_t WIFI_SCAN_TIMEOUT = 3000;            // when a Wifi scan was started and doesn't lead to a WIFI_EVENT_SCAN_DONE event it'll be discarded
  const uint32_t WIFI_DISCONNECT_TIMEOUT = 10000;     // if captivePortalActiveWhenNotConnected is set and the STA connection is lost the captive portal wil be started after this delay
  const uint32_t CAPTIVE_PORTAL_TIMEOUT_S = 300;      // if the captive portal was started manually it'll be stopped again after this delay

  // forward declarations
  void logCallback(int level, const char* tag, const char* message);
  void eventsOnConnect(AsyncEventSourceClient* client);
  void handleRoot(AsyncWebServerRequest* request);
  void handleStyle(AsyncWebServerRequest* request);
  void handleLogs(AsyncWebServerRequest* request);
  void handleConfig(AsyncWebServerRequest* request);
  void handleSafeConfig(AsyncWebServerRequest* request);
  void handleWifi(AsyncWebServerRequest* request);
  void handleSafeWifi(AsyncWebServerRequest* request);
  void handleScan(AsyncWebServerRequest* request);
  void handleReboot(AsyncWebServerRequest* request);
  void handleNotFound(AsyncWebServerRequest* request);
  bool handleCaptivePortal(AsyncWebServerRequest* request);
  String getStoredWiFiPass();
  String getStoredWiFiSsid();
  void scanWiFi(bool async);
  void scanWifiDone();
  const char* getEncType(uint8_t e);
  void stopCaptivePortal();
  void addStatus(String& page);

  DNSServer* dnsServer;
  AsyncWebServer server(HTTP_PORT);
  AsyncEventSource events("/events");

  volatile uint8_t wifiDisconnected = 0;
  uint32_t lastWifiReconnectAttempt = 0;
  uint32_t lastWifiDisconnect = 0;

  volatile bool captivePortalActive = false;
  uint32_t captivePortalTimeout = 0;
  static SemaphoreHandle_t captivePortalMutex = xSemaphoreCreateMutex();

  uint32_t lastWifiScan = 0;
  volatile bool wifiScanActive = false;
  static SemaphoreHandle_t scanWifiMutex = xSemaphoreCreateMutex();

  struct WiFiResult {
    bool duplicate;
    String SSID;
    uint8_t encryptionType;
    int32_t RSSI;
    uint8_t* BSSID;
    int32_t channel;
  };

  WiFiResult* foundWiFis;
  int16_t numberOfFoundWiFis;

  const char* appName;

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
    return (String(appName) + "-" + getMac());
  }

  std::vector<ConfigParameterBase<Config>*> configParameterVector;

  updateMessageCallback_t updateMessageCallback;
  setPriorityMessageCallback_t setPriorityMessageCallback;
  clearPriorityMessageCallback_t clearPriorityMessageCallback;
  configChangedCallback_t configChangedCallback;

  ImprovWiFi improvSerial(&Serial);

  void onImprovWiFiErrorCb(ImprovTypes::Error err) {
    ESP_LOGI(TAG, "onImprovWiFiErrorCb: %u", err);
  }

  void onImprovWiFiConnectedCb(const char* ssid, const char* password) {
    ESP_LOGI(TAG, "onImprovWiFiConnectedCb: %s", ssid);
  }

  bool improvConnectWifi(const char* _ssid, const char* _password) {
    ESP_LOGI(TAG, "onImprovWiFiConnectedCb: %s", ssid);
    lastWifiReconnectAttempt = millis();
    if (keepCaptivePortalActive) {
      WiFi.mode(WIFI_MODE_APSTA);
    } else {
      WiFi.mode(WIFI_MODE_STA);
    }
    WiFi.begin(_ssid, _password);
    return (WiFi.waitForConnectResult() == WL_CONNECTED);
  }

  /*
    const char* WIFI_STATUS_STR[] = {
        "WL_IDLE_STATUS",
        "WL_NO_SSID_AVAIL",
        "WL_SCAN_COMPLETED",
        "WL_CONNECTED",
        "WL_CONNECT_FAILED",
        "WL_CONNECTION_LOST",
        "WL_DISCONNECTED"
    };
  */

  void setupWifiManager(const char* _appName, std::vector<ConfigParameterBase<Config>*> _configParameterVector, bool _keepCaptivePortalActive, bool _captivePortalActiveWhenNotConnected,
    updateMessageCallback_t _updateMessageCallback, setPriorityMessageCallback_t _setPriorityMessageCallback, clearPriorityMessageCallback_t _clearPriorityMessageCallback,
    configChangedCallback_t _configChangedCallback) {
    appName = _appName;
    configParameterVector = _configParameterVector;
    keepCaptivePortalActive = _keepCaptivePortalActive;
    captivePortalActiveWhenNotConnected = _captivePortalActiveWhenNotConnected;
    updateMessageCallback = _updateMessageCallback;
    setPriorityMessageCallback = _setPriorityMessageCallback;
    clearPriorityMessageCallback = _clearPriorityMessageCallback;
    configChangedCallback = _configChangedCallback;

    // TODO: only if Wifi is configured
    WiFi.mode(WIFI_MODE_STA);
    ESP_LOGD(TAG, "Turning on Wifi, ssid: %s", getStoredWiFiSsid());
    lastWifiReconnectAttempt = millis();
    WiFi.begin();

    if (keepCaptivePortalActive) startCaptivePortal();

    // TODO: filebrowser for portable monitor? https://github.com/me-no-dev/ESPAsyncWebServer/blob/master/examples/ESP_AsyncFSBrowser/ESP_AsyncFSBrowser.ino
    // TODO: OTA, plus OTA status updates => https://github.com/me-no-dev/ESPAsyncWebServer/blob/master/examples/ESP_AsyncFSBrowser/ESP_AsyncFSBrowser.ino
    events.onConnect(eventsOnConnect);
    if (strlen(PORTAL_USER) > 0 && strlen(PORTAL_PW) > 0) events.setAuthentication(PORTAL_USER, PORTAL_PW);
    server.addHandler(&events);
    server.on("/", HTTP_GET, handleRoot);
    server.on("/styles.css", HTTP_GET, handleStyle);
    server.on("/logs", HTTP_GET, handleLogs);
    server.on("/config", HTTP_GET, handleConfig);
    server.on("/configsave", HTTP_GET, handleSafeConfig);
    server.on("/wifi", HTTP_GET, handleWifi);
    server.on("/wifisave", HTTP_GET, handleSafeWifi);
    server.on("/scan", HTTP_GET, handleScan);
    server.on("/reboot", HTTP_GET, handleReboot);
    server.onNotFound(handleNotFound);

    server.begin();
    logging::addOnLogCallback(logCallback);

    improvSerial.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32, appName, APP_VERSION, getSSID().c_str(), "");
    improvSerial.onImprovError(onImprovWiFiErrorCb);
    improvSerial.onImprovConnected(onImprovWiFiConnectedCb);
    improvSerial.setCustomConnectWiFi(improvConnectWifi);
  }

  bool authenticate(AsyncWebServerRequest* request) {
    if (strlen(PORTAL_USER) == 0 || strlen(PORTAL_PW) == 0) return true;
    if (!request->authenticate(PORTAL_USER, PORTAL_PW)) {
      request->requestAuthentication();
      return false;
    }
    return true;
  }

  void logCallback(int level, const char* tag, const char* message) {
    events.send(message, "log", millis());
  }

  void eventsOnConnect(AsyncEventSourceClient* client) {
    if (client->lastId()) {
      ESP_LOGD(TAG, "Client %u connected! Last received message is: %u", client->lastId());
    } else {
      ESP_LOGD(TAG, "Client %u connected!");
    }
  }

  void handleRoot(AsyncWebServerRequest* request) {
    ESP_LOGI(TAG, "handleRoot");
    String page = FPSTR(html::options_header);
    page.replace("{id}", getSSID());
    if (getStoredWiFiSsid() != "") {
      if (WiFi.status() == WL_CONNECTED) {
        page.replace("{wifi}", " on " + getStoredWiFiSsid());
      } else {
        page.replace("{wifi}", " <s>on " + getStoredWiFiSsid() + "</s>");
      }
    } else {
      page.replace("{wifi}", " No network configured.");
    }
    addStatus(page);
    page += FPSTR(html::options_footer);
    AsyncWebServerResponse* response = request->beginResponse(200, FPSTR(html::content_type_html), page);
    request->send(response);
  }

  void handleStyle(AsyncWebServerRequest* request) {
    ESP_LOGI(TAG, "handleStyle");
    if (!authenticate(request)) return;
    AsyncWebServerResponse* response = request->beginResponse(200, FPSTR("text/css"), FPSTR(html::style));
    response->addHeader(FPSTR(html::header_cache_control), "max-age=604800"); // max-age=604800 = 1 week
    request->send(response);
  }

  void handleLogs(AsyncWebServerRequest* request) {
    ESP_LOGI(TAG, "handleLogs");
    if (!authenticate(request)) return;
    AsyncWebServerResponse* response = request->beginResponse(200, FPSTR(html::content_type_html), FPSTR(html::logs));
    response->addHeader(FPSTR(html::header_access_control_allow_origin), FPSTR(html::cors_asterix));
    request->send(response);
  }

  void handleConfig(AsyncWebServerRequest* request) {
    ESP_LOGI(TAG, "handleConfig");
    if (!authenticate(request)) return;
    String page = FPSTR(html::config_header);
    char buf[8];
    for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
      String parameterHtml;
      if (configParameter->isNumber()) {
        parameterHtml = FPSTR(html::config_parameter_number);
        configParameter->getMinimum(buf);
        parameterHtml.replace("{mi}", buf);
        configParameter->getMaximum(buf);
        parameterHtml.replace("{ma}", buf);
        char defaultValue[configParameter->getMaxStrLen()];
        configParameter->print(config, defaultValue);
        parameterHtml.replace("{v}", defaultValue);
      } else if (configParameter->isBoolean()) {
        parameterHtml = FPSTR(html::config_parameter_checkbox);
        configParameter->print(config, buf);
        snprintf(buf, 8, "%s", strncmp(buf, "true", strlen(buf)) == 0 ? "checked" : "");
        parameterHtml.replace("{v}", buf);
      } else if (configParameter->isEnum()) {
        parameterHtml = FPSTR(html::config_parameter_select_start);
        configParameter->getMinimum(buf);
        uint16_t min = atoi(buf);
        configParameter->getMaximum(buf);
        uint16_t max = atoi(buf);
        for (uint16_t i = min; i <= max; i++) {
          parameterHtml += FPSTR(html::config_parameter_select_option);
          parameterHtml.replace("{v}", String(i).c_str());
          parameterHtml.replace("{lbl}", configParameter->getEnumLabels()[i]);
          if (i == configParameter->getValueOrdinal(config)) {
            parameterHtml.replace("{s}", "selected");
          } else {
            parameterHtml.replace("{s}", "");
          }
        }
        parameterHtml += FPSTR(html::config_parameter_select_end);
      } else {
        parameterHtml = FPSTR(html::config_parameter);
        char defaultValue[configParameter->getMaxStrLen()];
        configParameter->print(config, defaultValue);
        parameterHtml.replace("{v}", defaultValue);
      }
      parameterHtml.replace("{i}", configParameter->getId());
      parameterHtml.replace("{n}", configParameter->getId());
      parameterHtml.replace("{p}", configParameter->getLabel());
      snprintf(buf, 5, "%d", configParameter->getMaxStrLen());
      parameterHtml.replace("{l}", buf);
      page += parameterHtml;
    }
    page += FPSTR(html::config_footer);
    AsyncWebServerResponse* response = request->beginResponse(200, FPSTR(html::content_type_html), page);
    response->addHeader(FPSTR(html::header_cache_control), FPSTR(html::cache_control_no_cache));
    request->send(response);
  }

  void handleSafeConfig(AsyncWebServerRequest* request) {
    ESP_LOGI(TAG, "handleSafeConfig");
    if (!authenticate(request)) return;
    bool rebootRequired = false;
    for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
      rebootRequired |= configParameter->save(config, request->arg(configParameter->getId()).c_str()) && configParameter->isRebootRequiredOnChange();
    }
    logConfiguration(config);
    AsyncWebServerResponse* response = request->beginResponse(200, FPSTR(html::content_type_html), FPSTR(html::config_saved));
    response->addHeader(FPSTR(html::header_cache_control), FPSTR(html::cache_control_no_cache));
    request->send(response);

    if (wifiManagerTask) {
      if (rebootRequired)
        xTaskNotify(wifiManagerTask, X_CMD_SAVE_CONFIG_AND_REBOOT, eSetBits);
      else
        xTaskNotify(wifiManagerTask, X_CMD_SAVE_CONFIG, eSetBits);
    }
  }

  void handleWifi(AsyncWebServerRequest* request) {
    ESP_LOGI(TAG, "handleWifi");
    if (!authenticate(request)) return;
    if (numberOfFoundWiFis > 0 && (millis() - lastWifiScan < WIFI_SCAN_REUSE)) {
      // recent scan - use existing results
    } else {
      scanWiFi(true);
      ESP_LOGI(TAG, "handleWifi - after Scan");
    }
    String page;
    page += FPSTR(html::wifi_header);
    page += FPSTR(html::wifi_form);
    page += FPSTR(html::wifi_footer);
    AsyncWebServerResponse* response = request->beginResponse(200, FPSTR(html::content_type_html), page);
    request->send(response);
  }

  void handleSafeWifi(AsyncWebServerRequest* request) {
    ESP_LOGD(TAG, "Save Wifi: %s", request->arg("s").c_str());
    if (!authenticate(request)) return;

    strncpy(ssid, request->arg("s").c_str(), MAX_SSID_LEN);
    ssid[MAX_SSID_LEN] = 0x00;
    strncpy(password, request->arg("p").c_str(), MAX_PW_LEN);
    password[MAX_PW_LEN] = 0x00;

    String page;
    page += FPSTR(html::wifi_saved);
    AsyncWebServerResponse* response = request->beginResponse(200, FPSTR(html::content_type_html), page);
    response->addHeader(FPSTR(html::header_cache_control), FPSTR(html::cache_control_no_cache));
    request->send(response);

    if (wifiManagerTask)
      xTaskNotify(wifiManagerTask, X_CMD_CONNECT, eSetBits);
  }

  void addStatus(String& page) {
    if (getStoredWiFiSsid() != "") {
      page += F("Configured to connect to AP <b>");
      page += getStoredWiFiSsid();

      if (WiFi.status() == WL_CONNECTED) {
        page += F(" and connected</b> on IP <a href=\"http://");
        page += WiFi.localIP().toString();
        page += F("/\">");
        page += WiFi.localIP().toString();
        page += F("</a>");
      } else {
        page += F(" but not connected.</b>");
      }
    } else {
      page += F("No network configured.");
    }
  }

  void resetSettings() {
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_MODE_NULL);
  }

  String getStoredWiFiSsid() {
    if (WiFi.getMode() == WIFI_MODE_NULL) {
      return String();
    }
    wifi_ap_record_t info;
    if (!esp_wifi_sta_get_ap_info(&info)) {
      return String(reinterpret_cast<char*>(info.ssid));
    } else {
      wifi_config_t conf;
      esp_wifi_get_config(WIFI_IF_STA, &conf);
      return String(reinterpret_cast<char*>(conf.sta.ssid));
    }
    return String();
  }

  String getStoredWiFiPass() {
    if (WiFi.getMode() == WIFI_MODE_NULL) {
      return String();
    }

    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    return String(reinterpret_cast<char*>(conf.sta.password));
  }

  bool isIp(const String& str) {
    for (unsigned int i = 0; i < str.length(); i++) {
      int c = str.charAt(i);
      if (c != '.' && c != ':' && (c < '0' || c > '9')) {
        return false;
      }
    }
    return true;
  }

  String toStringIp(const IPAddress& ip) {
    String res = "";
    for (int i = 0; i < 3; i++) {
      res += String((ip >> (8 * i)) & 0xFF) + ".";
    }
    res += String(((ip >> 8 * 3)) & 0xFF);
    return res;
  }

  void startCaptivePortal() {
    boolean result = captivePortalMutex != NULL && (xSemaphoreTake(captivePortalMutex, pdMS_TO_TICKS(500)) == pdTRUE);
    if (!result) {
      ESP_LOGE(TAG, "Failed to obtain captivePortal mutex");
      return;
    }
    if (captivePortalActive) {
      xSemaphoreGive(captivePortalMutex);
      return;
    }
    captivePortalActive = true;
    xSemaphoreGive(captivePortalMutex);
    captivePortalTimeout = millis();
    ESP_LOGI(TAG, "Starting Captive portal SSID: %s", getSSID().c_str());
    setPriorityMessageCallback(getSSID().c_str());

    scanWiFi(true);

    WiFi.softAPConfig(IPAddress(192, 168, 100, 1), IPAddress(192, 168, 100, 1), IPAddress(255, 255, 255, 0));

    const char* pw = AP_PW;
    if (pw != NULL) {
      if (strlen(pw) < 8 || strlen(pw) > 63) {
        ESP_LOGE(TAG, "Invalid AccessPoint password. Ignoring");
        pw = NULL;
      }
    }
    if (pw != NULL) {
      WiFi.softAP(getSSID().c_str(), pw);
    } else {
      WiFi.softAP(getSSID().c_str());
    }
    if (!dnsServer) {
      dnsServer = new DNSServer();
      dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
      if (!dnsServer->start(53, "*", WiFi.softAPIP())) {
        ESP_LOGE(TAG, "Can't start DNS Server. No available socket");
      }
    }
  }

  bool handleCaptivePortal(AsyncWebServerRequest* request) {
    ESP_LOGI(TAG, "handleCaptivePortal %s", request->url().c_str());
    if (!isIp(request->host())) {

      ESP_LOGI(TAG, "Request redirected to captive portal");

      AsyncWebServerResponse* response = request->beginResponse(302, html::content_type_plain, "");
      response->addHeader("Location", String("http://") + toStringIp(request->client()->localIP()));
      request->send(response);
      return true;
    }

    ESP_LOGD(TAG, "request host IP = %s", request->host());

    return false;
  }

  void handleNotFound(AsyncWebServerRequest* request) {
    ESP_LOGI(TAG, "handleNotFound %s", request->url().c_str());
    if (ON_AP_FILTER(request) && handleCaptivePortal(request)) {
      return;
    }

    AsyncWebServerResponse* response = request->beginResponse(302, html::content_type_plain, "");
    response->addHeader("Location", "/");
    response->addHeader(FPSTR(html::header_cache_control), FPSTR(html::cache_control_no_cache));
    request->send(response);
  }

  uint8_t rssiToPercentage(int rssi) {
    if (rssi <= -100) {
      return 0;
    } else if (rssi >= -50) {
      return 100;
    } else {
      return 2 * (rssi + 100);
    }
  }

  void handleScan(AsyncWebServerRequest* request) {
    ESP_LOGD(TAG, "handleScan()");
    if (!authenticate(request)) return;
    bool force = (request->arg("f") == "1");
    uint32_t now = millis();
    if (wifiScanActive) {
      // wait until current scan is done
      while (wifiScanActive && (millis() - now < WIFI_SCAN_TIMEOUT)) vTaskDelay(pdMS_TO_TICKS(10));
    } else {
      if (!force && numberOfFoundWiFis > 0 && (millis() - lastWifiScan < WIFI_SCAN_REUSE)) {
        // recent scan - use existing results, unless force
      } else {
        scanWiFi(false);
        ESP_LOGI(TAG, "handleScan - after scan");
      }
    }
    String page = F("[");
    for (int i = 0;i < numberOfFoundWiFis;i++) {
      if (!foundWiFis[i].duplicate) {
        if (i != 0) page += F(", ");
        String item = "{\"ssid\":\"{v}\", \"enc\":\"{i}\", \"rssi\":\"{r}\"}";
        item.replace("{v}", foundWiFis[i].SSID);
        char buf[6] = { 0 };
        snprintf(buf, 5, "%u", rssiToPercentage(foundWiFis[i].RSSI));
        item.replace("{r}", buf);
        snprintf(buf, 2, "%d", foundWiFis[i].encryptionType);
        item.replace("{i}", buf);
        page += item;
      }
    }
    page += F("]");
    AsyncWebServerResponse* response = request->beginResponse(200, html::content_type_json, page);
    response->addHeader(FPSTR(html::header_cache_control), FPSTR(html::cache_control_no_cache));
    response->addHeader(FPSTR(html::header_access_control_allow_origin), FPSTR(html::cors_asterix));
    request->send(response);
  }

  const char* getEncType(uint8_t e) {
    switch (e) {
      case 0:
        return "WIFI_AUTH_OPEN";
      case 1:
        return "WIFI_AUTH_WEP";
      case 2:
        return "WIFI_AUTH_WPA_PSK";
      case 3:
        return "WIFI_AUTH_WPA2_PSK";
      case 4:
        return "WIFI_AUTH_WPA_WPA2_PSK";
      case 5:
        return "WIFI_AUTH_WPA2_ENTERPRISE";
      case 6:
        return "WIFI_AUTH_WPA3_PSK";
      case 7:
        return "WIFI_AUTH_WPA2_WPA3_PSK";
      case 8:
        return "WIFI_AUTH_WAPI_PSK";
      default:
        return"INVALID";
    }
  }

  void scanWifiDone() {
    ESP_LOGD(TAG, "Scan done... numberOfFoundWiFis: %d", numberOfFoundWiFis);
    if (foundWiFis) delete[] foundWiFis;
    if (numberOfFoundWiFis < 0) {
      foundWiFis = new WiFiResult[0];
      wifiScanActive = false;
      return;
    }
    foundWiFis = new WiFiResult[numberOfFoundWiFis];

    for (int i = 0; i < numberOfFoundWiFis; i++) {
      foundWiFis[i].duplicate = false;
      bool res = WiFi.getNetworkInfo(i, foundWiFis[i].SSID, foundWiFis[i].encryptionType, foundWiFis[i].RSSI, foundWiFis[i].BSSID, foundWiFis[i].channel);
      if (!res) ESP_LOGE(TAG, "error retrieving network: %u", i);
      ESP_LOGD(TAG, "Found network: %s, e: %s RSSI: %i, channel: %i", foundWiFis[i].SSID, getEncType(foundWiFis[i].encryptionType), foundWiFis[i].RSSI, foundWiFis[i].channel);
    }
    // sort
    for (int i = 0; i < numberOfFoundWiFis; i++) {
      for (int j = i + 1; j < numberOfFoundWiFis; j++) {
        if (foundWiFis[j].RSSI > foundWiFis[i].RSSI) {
          std::swap(foundWiFis[i], foundWiFis[j]);
        }
      }
    }
    String cssid;
    for (int i = 0; i < numberOfFoundWiFis; i++) {
      if (foundWiFis[i].duplicate == true)
        continue;
      cssid = foundWiFis[i].SSID;
      for (int j = i + 1; j < numberOfFoundWiFis; j++) {
        if (cssid == foundWiFis[j].SSID) {
          ESP_LOGD(TAG, "Duplicate SSID: %s", foundWiFis[j].SSID);
          foundWiFis[j].duplicate = true;
        }
      }
    }
    wifiScanActive = false;
  }

  void scanWiFi(bool async) {
    boolean result = scanWifiMutex != NULL && (xSemaphoreTake(scanWifiMutex, pdMS_TO_TICKS(500)) == pdTRUE);
    if (!result) {
      ESP_LOGE(TAG, "Failed to obtain wifiScan mutex");
      return;
    }
    if (wifiScanActive) {
      xSemaphoreGive(scanWifiMutex);
      return;
    }
    wifiScanActive = true;
    xSemaphoreGive(scanWifiMutex);
    ESP_LOGD(TAG, "Start scan %s", async ? "async" : "sync");
    lastWifiScan = millis();
    numberOfFoundWiFis = WIFI_SCAN_RUNNING;
    numberOfFoundWiFis = WiFi.scanNetworks(async, true, false, 1500);

    uint32_t now = millis();
    if (!async) {
      // wait until current scan is done
      while (wifiScanActive && (millis() - now < WIFI_SCAN_TIMEOUT)) vTaskDelay(pdMS_TO_TICKS(100));
    }
  }

  const char* WIFI_EVENT_STRINGS[] = {
      "WIFI_EVENT_WIFI_READY",           /**< ESP32 WiFi ready */
      "WIFI_EVENT_SCAN_DONE",                /**< ESP32 finish scanning AP */
      "WIFI_EVENT_STA_START",                /**< ESP32 station start */
      "WIFI_EVENT_STA_STOP",                 /**< ESP32 station stop */
      "WIFI_EVENT_STA_CONNECTED",            /**< ESP32 station connected to AP */
      "WIFI_EVENT_STA_DISCONNECTED",         /**< ESP32 station disconnected from AP */
      "WIFI_EVENT_STA_AUTHMODE_CHANGE",      /**< the auth mode of AP connected by ESP32 station changed */

      "WIFI_EVENT_STA_WPS_ER_SUCCESS",       /**< ESP32 station wps succeeds in enrollee mode */
      "WIFI_EVENT_STA_WPS_ER_FAILED",        /**< ESP32 station wps fails in enrollee mode */
      "WIFI_EVENT_STA_WPS_ER_TIMEOUT",       /**< ESP32 station wps timeout in enrollee mode */
      "WIFI_EVENT_STA_WPS_ER_PIN",           /**< ESP32 station wps pin code in enrollee mode */
      "WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP",   /**< ESP32 station wps overlap in enrollee mode */

      "WIFI_EVENT_AP_START",                 /**< ESP32 soft-AP start */
      "WIFI_EVENT_AP_STOP",                  /**< ESP32 soft-AP stop */
      "WIFI_EVENT_AP_STACONNECTED",          /**< a station connected to ESP32 soft-AP */
      "WIFI_EVENT_AP_STADISCONNECTED",       /**< a station disconnected from ESP32 soft-AP */
      "WIFI_EVENT_AP_PROBEREQRECVED",        /**< Receive probe request packet in soft-AP interface */

      "WIFI_EVENT_FTM_REPORT",               /**< Receive report of FTM procedure */

      /* Add next events after this only */
      "WIFI_EVENT_STA_BSS_RSSI_LOW",         /**< AP's RSSI crossed configured threshold */
      "WIFI_EVENT_ACTION_TX_STATUS",         /**< Status indication of Action Tx operation */
      "WIFI_EVENT_ROC_DONE",                 /**< Remain-on-Channel operation complete */

      "WIFI_EVENT_STA_BEACON_TIMEOUT",       /**< ESP32 station beacon timeout */
  };

  const char* system_event_reasons[] = { "UNSPECIFIED", "AUTH_EXPIRE", "AUTH_LEAVE", "ASSOC_EXPIRE", "ASSOC_TOOMANY", "NOT_AUTHED", "NOT_ASSOCED", "ASSOC_LEAVE", "ASSOC_NOT_AUTHED", "DISASSOC_PWRCAP_BAD", "DISASSOC_SUPCHAN_BAD", "UNSPECIFIED", "IE_INVALID", "MIC_FAILURE", "4WAY_HANDSHAKE_TIMEOUT", "GROUP_KEY_UPDATE_TIMEOUT", "IE_IN_4WAY_DIFFERS", "GROUP_CIPHER_INVALID", "PAIRWISE_CIPHER_INVALID", "AKMP_INVALID", "UNSUPP_RSN_IE_VERSION", "INVALID_RSN_IE_CAP", "802_1X_AUTH_FAILED", "CIPHER_SUITE_REJECTED", "BEACON_TIMEOUT", "NO_AP_FOUND", "AUTH_FAIL", "ASSOC_FAIL", "HANDSHAKE_TIMEOUT", "CONNECTION_FAIL" };
#define reason2str(r) ((r>176)?system_event_reasons[r-176]:system_event_reasons[r-1])

  void eventHandler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
      ESP_LOGD(TAG, "eventHandler IP_EVENT IP_EVENT_STA_GOT_IP");
      ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
      ESP_LOGD(TAG, "STA Got %sIP:" IPSTR, event->ip_changed ? "New " : "Same ", IP2STR(&event->ip_info.ip));
      Timekeeper::initSntp();
    } else if (event_base == WIFI_EVENT) {
      if (event_id > 0 && event_id < WIFI_EVENT_MAX) {
        ESP_LOGD(TAG, "eventHandler %s", WIFI_EVENT_STRINGS[event_id]);
      } else {
        ESP_LOGD(TAG, "eventHandler - unknown id: %d", event_id);
      }
      if (event_id == WIFI_EVENT_STA_CONNECTED) {
        wifiDisconnected = 0;
        if (LED_PIN >= 0) digitalWrite(LED_PIN, HIGH);
        if (captivePortalActive) {
          stopCaptivePortal();
          WiFi.setAutoReconnect(true);
        }
      } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifiDisconnected = 1;
        if (LED_PIN >= 0) digitalWrite(LED_PIN, LOW);
        lastWifiDisconnect = millis();
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*)event_data;
        ESP_LOGD(TAG, "STA Disconnected: ssid: %s, reason: %s", event->ssid, reason2str(event->reason));

        if (event->reason == 15) {
          WiFi.setAutoReconnect(false);
        }

      } else if (event_id == WIFI_EVENT_SCAN_DONE) {
        wifi_event_sta_scan_done_t* event = (wifi_event_sta_scan_done_t*)event_data;
        ESP_LOGD(TAG, "Scan done: id: %u, status: %u, results: %u", event->scan_id, event->status, event->number);
        numberOfFoundWiFis = event->number;
        if (wifiManagerTask)
          xTaskNotify(wifiManagerTask, X_CMD_WIFI_SCAN_DONE, eSetBits);
      } else {
        //          ESP_LOGD(TAG, "eventHandler WIFI_EVENT %u", event_id);
      }
    } else {
      ESP_LOGD(TAG, "eventHandler %s %u", event_base, event_id);
    }
  }

  /*
  Wrong PW: (on start-up)
  Reason 15 4WAY-handshake

  SSID drop:
  Reason: 1 - UNSPECIFIED

  SSID not found:
  201 - NO_AP_FOUND
  */

  void stopCaptivePortal() {
    if (keepCaptivePortalActive) return;
    captivePortalActive = false;
    clearPriorityMessageCallback();
    if (dnsServer) {
      ESP_LOGD(TAG, "Stopping and deleting dnsServer");
      dnsServer->stop();
      delete dnsServer;
      dnsServer = nullptr;
    }
    WiFi.mode(WIFI_MODE_STA);
  }

  void handleReboot(AsyncWebServerRequest* request) {
    if (!authenticate(request)) return;
    AsyncWebServerResponse* response = request->beginResponse(200, FPSTR(html::content_type_html), FPSTR(html::reboot));
    request->send(response);
    delay(1000);
    esp_restart();
  }

  void wifiManagerLoop(void* pvParameters) {
    _ASSERT((uint32_t)pvParameters == 1);
    BaseType_t notified;
    uint32_t taskNotification;
    while (1) {
      notified = xTaskNotifyWait(0x00,  // Don't clear any bits on entry
        ULONG_MAX,                      // Clear all bits on exit
        &taskNotification,              // Receives the notification value
        pdMS_TO_TICKS(50));
      if (notified == pdPASS) {
        if (taskNotification & X_CMD_CONNECT) {
          ESP_LOGD(TAG, "X_CMD_CONNECT");
          lastWifiReconnectAttempt = millis();
          if (keepCaptivePortalActive) {
            WiFi.mode(WIFI_MODE_APSTA);
          } else {
            WiFi.mode(WIFI_MODE_STA);
          }
          WiFi.begin(ssid, password);
        }
        if (taskNotification & X_CMD_SAVE_CONFIG) {
          saveConfiguration(config);
          configChangedCallback();
        }
        if (taskNotification & X_CMD_SAVE_CONFIG_AND_REBOOT) {
          if (saveConfiguration(config)) {
            delay(1000);
            esp_restart();
          }
        }
        if (taskNotification & X_CMD_WIFI_SCAN_DONE) {
          scanWifiDone();
        }
      }
      if (captivePortalActive) {
        if (dnsServer) {
          dnsServer->processNextRequest();
        }
        if (!keepCaptivePortalActive && (!wifiDisconnected || !captivePortalActiveWhenNotConnected || !(lastWifiDisconnect - millis() > WIFI_DISCONNECT_TIMEOUT))) {
          if (millis() - captivePortalTimeout > (uint32_t)(CAPTIVE_PORTAL_TIMEOUT_S * 1000)) {
            ESP_LOGD(TAG, "Captive portal timed out - stopping");
            stopCaptivePortal();
          }
        }
      } else {
        if (wifiDisconnected && captivePortalActiveWhenNotConnected && (lastWifiDisconnect - millis() > WIFI_DISCONNECT_TIMEOUT)) {
          startCaptivePortal();
        }
        if (wifiDisconnected == 1 && !WiFi.isConnected() && WiFi.getMode() == WIFI_MODE_STA) {
          /*
          if (millis() - lastWifiReconnectAttempt >= 60000) {
            ESP_LOGD(TAG, "WiFi.begin()");
            WiFi.begin();
            lastWifiReconnectAttempt = millis();
          }*/
        }
      }
      if (wifiScanActive && (millis() - lastWifiScan > 5000)) {
        int16_t status = WiFi.scanComplete();
        ESP_LOGD(TAG, "Scan timeout? %d", status);
        if (status != WIFI_SCAN_RUNNING) {
          numberOfFoundWiFis = status;
          scanWifiDone();
        }
      }
      improvSerial.handleSerial();
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    vTaskDelete(NULL);
  }

  TaskHandle_t start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core) {
    xTaskCreatePinnedToCore(
      wifiManagerLoop,  // task function
      name,             // name of task
      stackSize,        // stack size of task
      (void*)1,         // parameter of the task
      priority,         // priority of the task
      &wifiManagerTask, // task handle
      core);            // CPU core
    return wifiManagerTask;
  }

}