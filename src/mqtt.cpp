#include <mqtt.h>
#include <Arduino.h>
#include <config.h>

#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <i2c.h>
#include <configManager.h>
#include <model.h>
#include <wifiManager.h>
#include <ota.h>

#include <LittleFS.h>

#include <ArduinoJson.h>

// Local logging tag
static const char TAG[] = __FILE__;

namespace mqtt {

  struct MqttMessage {
    uint8_t cmd;
    uint16_t mask;
    const char* statusMessage;
  };

  const uint8_t X_CMD_PUBLISH_SENSORS = bit(0);
  const uint8_t X_CMD_PUBLISH_CONFIGURATION = bit(1);
  const uint8_t X_CMD_PUBLISH_STATUS_MSG = bit(2);

  TaskHandle_t mqttTask;
  QueueHandle_t mqttQueue;

  WiFiClient* wifiClient;
  PubSubClient* mqtt_client;
  Model* model;

  calibrateCo2SensorCallback_t calibrateCo2SensorCallback;
  setTemperatureOffsetCallback_t setTemperatureOffsetCallback;
  getTemperatureOffsetCallback_t getTemperatureOffsetCallback;
  getSPS30AutoCleanIntervalCallback_t getSPS30AutoCleanIntervalCallback;
  setSPS30AutoCleanIntervalCallback_t setSPS30AutoCleanIntervalCallback;
  cleanSPS30Callback_t cleanSPS30Callback;
  getSPS30StatusCallback_t getSPS30StatusCallback;

  uint32_t lastReconnectAttempt = 0;
  uint16_t connectionAttempts = 0;

  StaticJsonDocument<CONFIG_SIZE> doc;

  void publishSensors(uint16_t mask) {
    if (!WiFi.isConnected() || !mqtt_client->connected()) return;
    MqttMessage msg;
    msg.cmd = X_CMD_PUBLISH_SENSORS;
    msg.mask = mask;
    if (mqttQueue) xQueueSendToBack(mqttQueue, (void*)&msg, pdMS_TO_TICKS(100));
  }

  void publishSensorsInternal(uint16_t mask) {
    char topic[256];
    char msg[256];
    sprintf(topic, "%s/%u/up/sensors", config.mqttTopic, config.deviceId);

    char buf[8];
    doc.clear();
    if (mask & M_CO2) doc["co2"] = model->getCo2();
    if (mask & M_TEMPERATURE) {
      sprintf(buf, "%.1f", model->getTemperature());
      doc["temperature"] = buf;
    }
    if (mask & M_HUMIDITY) {
      sprintf(buf, "%.1f", model->getHumidity());
      doc["humidity"] = buf;
    }
    if (mask & M_PRESSURE) doc["pressure"] = model->getPressure();
    if (mask & M_IAQ) doc["iaq"] = model->getIAQ();
    if (mask & M_PM0_5) doc["pm0.5"] = model->getPM0_5();
    if (mask & M_PM1_0) doc["pm1"] = model->getPM1();
    if (mask & M_PM2_5) doc["pm2.5"] = model->getPM2_5();
    if (mask & M_PM4) doc["pm4"] = model->getPM4();
    if (mask & M_PM10) doc["pm10"] = model->getPM10();

    // Serialize JSON to file
    if (serializeJson(doc, msg) == 0) {
      ESP_LOGW(TAG, "Failed to serialise payload");
      return;
    }
    if (strncmp(msg, "null", 4) == 0) {
      ESP_LOGD(TAG, "Nothing to publish - mask: %x", mask);
      return;
    }
    ESP_LOGD(TAG, "Publishing sensor values: %s:%s", topic, msg);
    if (!mqtt_client->publish(topic, msg)) ESP_LOGE(TAG, "publish sensors failed!");
  }

  void publishConfiguration() {
    MqttMessage msg;
    msg.cmd = X_CMD_PUBLISH_CONFIGURATION;
    msg.mask = 0;
    msg.statusMessage = nullptr;
    if (mqttQueue) xQueueSendToBack(mqttQueue, (void*)&msg, pdMS_TO_TICKS(100));
  }

  void publishConfigurationInternal() {
    char buf[256];
    char msg[CONFIG_SIZE];
    doc.clear();
    doc["appVersion"] = APP_VERSION;
    doc["mqttHost"] = config.mqttHost;
    doc["mqttServerPort"] = config.mqttServerPort;
    doc["mqttUsername"] = config.mqttUsername;
    doc["mqttTopic"] = config.mqttTopic;
    doc["mqttUseTls"] = config.mqttUseTls;
    doc["mqttInsecure"] = config.mqttInsecure;
    doc["altitude"] = config.altitude;
    doc["co2YellowThreshold"] = config.co2YellowThreshold;
    doc["co2RedThreshold"] = config.co2RedThreshold;
    doc["co2DarkRedThreshold"] = config.co2DarkRedThreshold;
    doc["iaqYellowThreshold"] = config.iaqYellowThreshold;
    doc["iaqRedThreshold"] = config.iaqRedThreshold;
    doc["iaqDarkRedThreshold"] = config.iaqDarkRedThreshold;
    doc["brightness"] = config.brightness;
    sprintf(buf, "%s", WifiManager::getMac().c_str());
    doc["mac"] = buf;
    sprintf(buf, "%s", WiFi.localIP().toString().c_str());
    doc["ip"] = buf;
    if (I2C::scd30Present())
      doc["scd30"] = true;
    if (I2C::scd40Present())
      doc["scd40"] = true;
    if (I2C::bme680Present())
      doc["bme680"] = true;
    if (I2C::lcdPresent())
      doc["lcd"] = true;
    if (I2C::sps30Present()) {
      doc["sps30"] = true;
      doc["sps30AutoCleanInt"] = getSPS30AutoCleanIntervalCallback();
      doc["sps30Status"] = getSPS30StatusCallback();
    }
    doc["ssd1306Rows"] = config.ssd1306Rows;
    doc["greenLed"] = config.greenLed;
    doc["yellowLed"] = config.yellowLed;
    doc["redLed"] = config.redLed;
    doc["neopixelData"] = config.neopixelData;
    doc["neopixelNumber"] = config.neopixelNumber;
    doc["neopixelMatrixData"] = config.neopixelMatrixData;
    doc["featherMatrixData"] = config.featherMatrixData;
    doc["featherMatrixClock"] = config.featherMatrixClock;
    doc["matrixColumns"] = config.matrixColumns;
    doc["matrixRows"] = config.matrixRows;
    doc["matrixLayout"] = config.matrixLayout;
    doc["hub75R1"] = config.hub75R1;
    doc["hub75G1"] = config.hub75G1;
    doc["hub75B1"] = config.hub75B1;
    doc["hub75R2"] = config.hub75R2;
    doc["hub75G2"] = config.hub75G2;
    doc["hub75B2"] = config.hub75B2;
    doc["hub75ChA"] = config.hub75ChA;
    doc["hub75ChB"] = config.hub75ChB;
    doc["hub75ChC"] = config.hub75ChC;
    doc["hub75ChD"] = config.hub75ChD;
    doc["hub75Clk"] = config.hub75Clk;
    doc["hub75Lat"] = config.hub75Lat;
    doc["hub75Oe"] = config.hub75Oe;
    sprintf(buf, "%.1f", getTemperatureOffsetCallback());
    doc["tempOffset"] = buf;
    if (serializeJson(doc, msg) == 0) {
      ESP_LOGW(TAG, "Failed to serialise payload");
      return;
    }
    sprintf(buf, "%s/%u/up/config", config.mqttTopic, config.deviceId);
    ESP_LOGI(TAG, "Publishing configuration: %s:%s", buf, msg);
    if (!mqtt_client->publish(buf, msg)) ESP_LOGE(TAG, "publish configuration failed!");
  }

  void publishStatusMsg(const char* statusMessage) {
    MqttMessage msg;
    msg.cmd = X_CMD_PUBLISH_STATUS_MSG;
    msg.mask = 0;
    msg.statusMessage = statusMessage;
    if (mqttQueue) xQueueSendToBack(mqttQueue, (void*)&msg, pdMS_TO_TICKS(100));
  }

  void publishStatusMsgInternal(const char* statusMessage) {
    if (strlen(statusMessage) > 200) return;
    char topic[256];
    sprintf(topic, "%s/%u/up/status", config.mqttTopic, config.deviceId);
    char msg[256];
    doc.clear();
    doc["msg"] = statusMessage;
    if (serializeJson(doc, msg) == 0) {
      ESP_LOGW(TAG, "Failed to serialise payload");
      return;
    }
    if (!mqtt_client->publish(topic, msg)) ESP_LOGE(TAG, "publish status msg failed!");
  }

  void callback(char* topic, byte* payload, unsigned int length) {
    char buf[256];
    char msg[length + 1];
    strncpy(msg, (char*)payload, length);
    msg[length] = 0x00;
    ESP_LOGI(TAG, "Message arrived [%s] %s", topic, msg);

    sprintf(buf, "%s/%u/down/", config.mqttTopic, config.deviceId);
    int16_t cmdIdx = -1;
    if (strncmp(topic, buf, strlen(buf)) == 0) {
      ESP_LOGI(TAG, "Device specific downlink message arrived [%s]", topic);
      cmdIdx = strlen(buf);
    }
    sprintf(buf, "%s/down/", config.mqttTopic);
    if (strncmp(topic, buf, strlen(buf)) == 0) {
      ESP_LOGI(TAG, "Device agnostic downlink message arrived [%s]", topic);
      cmdIdx = strlen(buf);
    }
    if (cmdIdx < 0) return;
    strncpy(buf, topic + cmdIdx, strlen(topic) - cmdIdx + 1);
    ESP_LOGI(TAG, "Received command [%s]", buf);

    if (strncmp(buf, "calibrate", strlen(buf)) == 0) {
      int reference = atoi(msg);
      if (reference >= 400 && reference <= 2000) {
        calibrateCo2SensorCallback(reference);
      }
    } else if (strncmp(buf, "setTemperatureOffset", strlen(buf)) == 0) {
      float tempOffset = atof(msg);
      if (0 <= tempOffset && tempOffset <= 10.0) {
        setTemperatureOffsetCallback(tempOffset);
      }
    } else if (strncmp(buf, "setSPS30AutoCleanInterval", strlen(buf)) == 0) {
      char* eptr;
      long interval = std::strtoul(msg, &eptr, 10);
      setSPS30AutoCleanIntervalCallback(interval);
    } else if (strncmp(buf, "cleanSPS30", strlen(buf)) == 0) {
      cleanSPS30Callback();
    } else if (strncmp(buf, "getConfig", strlen(buf)) == 0) {
      publishConfiguration();
    } else if (strncmp(buf, "setConfig", strlen(buf)) == 0) {
      doc.clear();
      DeserializationError error = deserializeJson(doc, msg);
      if (error) {
        ESP_LOGW(TAG, "Failed to parse message: %s", error.f_str());
        return;
      }
      bool rebootRequired = false;
      if (doc.containsKey("altitude")) config.altitude = doc["altitude"].as<int>();
      if (doc.containsKey("co2YellowThreshold")) config.co2YellowThreshold = doc["co2YellowThreshold"].as<uint16_t>();
      if (doc.containsKey("co2RedThreshold")) config.co2RedThreshold = doc["co2RedThreshold"].as<uint16_t>();
      if (doc.containsKey("co2DarkRedThreshold")) config.co2DarkRedThreshold = doc["co2DarkRedThreshold"].as<uint16_t>();
      if (doc.containsKey("iaqYellowThreshold")) config.iaqYellowThreshold = doc["iaqYellowThreshold"].as<uint16_t>();
      if (doc.containsKey("iaqRedThreshold")) config.iaqRedThreshold = doc["iaqRedThreshold"].as<uint16_t>();
      if (doc.containsKey("iaqDarkRedThreshold")) config.iaqDarkRedThreshold = doc["iaqDarkRedThreshold"].as<uint16_t>();
      if (doc.containsKey("brightness")) config.brightness = doc["brightness"].as<uint8_t>();
      if (doc.containsKey("ssd1306Rows")) { config.ssd1306Rows = doc["ssd1306Rows"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("greenLed")) { config.greenLed = doc["greenLed"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("yellowLed")) { config.yellowLed = doc["yellowLed"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("redLed")) { config.redLed = doc["redLed"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("neopixelData")) { config.neopixelData = doc["neopixelData"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("neopixelNumber")) { config.neopixelNumber = doc["neopixelNumber"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("neopixelMatrixData")) { config.neopixelMatrixData = doc["neopixelMatrixData"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("featherMatrixData")) { config.featherMatrixData = doc["featherMatrixData"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("featherMatrixClock")) { config.featherMatrixClock = doc["featherMatrixClock"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("matrixColumns")) { config.matrixColumns = doc["matrixColumns"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("matrixRows")) { config.matrixRows = doc["matrixRows"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("matrixLayout")) { config.matrixLayout = doc["matrixLayout"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("hub75R1")) { config.hub75R1 = doc["hub75R1"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("hub75G1")) { config.hub75G1 = doc["hub75G1"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("hub75B1")) { config.hub75B1 = doc["hub75B1"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("hub75R2")) { config.hub75R2 = doc["hub75R2"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("hub75G2")) { config.hub75G2 = doc["hub75G2"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("hub75B2")) { config.hub75B2 = doc["hub75B2"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("hub75ChA")) { config.hub75ChA = doc["hub75ChA"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("hub75ChB")) { config.hub75ChB = doc["hub75ChB"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("hub75ChC")) { config.hub75ChC = doc["hub75ChC"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("hub75ChD")) { config.hub75ChD = doc["hub75ChD"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("hub75Clk")) { config.hub75Clk = doc["hub75Clk"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("hub75Lat")) { config.hub75Lat = doc["hub75Lat"].as<uint8_t>(); rebootRequired = true; }
      if (doc.containsKey("hub75Oe")) { config.hub75Oe = doc["hub75Oe"].as<uint8_t>(); rebootRequired = true; }

      Config mqttConfig = config;
      bool mqttConfigUpdated = false;
      bool mqttTestSuccess = true;
      if (doc.containsKey("mqttHost")) { strlcpy(mqttConfig.mqttHost, doc["mqttHost"], sizeof(config.mqttHost)); mqttConfigUpdated = true; }
      if (doc.containsKey("mqttServerPort")) { mqttConfig.mqttServerPort = doc["mqttServerPort"].as<uint16_t>(); mqttConfigUpdated = true; }
      if (doc.containsKey("mqttUsername")) { strlcpy(mqttConfig.mqttUsername, doc["mqttUsername"], sizeof(config.mqttUsername)); mqttConfigUpdated = true; }
      if (doc.containsKey("mqttPassword")) { strlcpy(mqttConfig.mqttPassword, doc["mqttPassword"], sizeof(config.mqttPassword)); mqttConfigUpdated = true; }
      if (doc.containsKey("mqttTopic")) { strlcpy(mqttConfig.mqttTopic, doc["mqttTopic"], sizeof(config.mqttTopic)); mqttConfigUpdated = true; }
      if (doc.containsKey("mqttUseTls")) { mqttConfig.mqttUseTls = doc["mqttUseTls"].as<boolean>(); mqttConfigUpdated = true; }
      if (doc.containsKey("mqttInsecure")) { mqttConfig.mqttInsecure = doc["mqttInsecure"].as<boolean>(); mqttConfigUpdated = true; }

      if (mqttConfigUpdated) {
        WiFiClient* testWifiClient;
        if (mqttConfig.mqttUseTls) {
          testWifiClient = new WiFiClientSecure();
          if (mqttConfig.mqttInsecure) {
            ((WiFiClientSecure*)testWifiClient)->setInsecure();
          }
          File root_ca_file = LittleFS.open(MQTT_ROOT_CA_FILENAME, "r");
          if (root_ca_file) {
            ESP_LOGD(TAG, "Loading MQTT root ca from FS (%s)", MQTT_ROOT_CA_FILENAME);
            ((WiFiClientSecure*)testWifiClient)->loadCACert(root_ca_file, root_ca_file.size());
            root_ca_file.close();
          }
          File client_key_file = LittleFS.open(MQTT_CLIENT_KEY_FILENAME, "r");
          if (client_key_file) {
            ESP_LOGD(TAG, "Loading MQTT client key from FS (%s)", MQTT_CLIENT_KEY_FILENAME);
            ((WiFiClientSecure*)testWifiClient)->loadPrivateKey(client_key_file, client_key_file.size());
            client_key_file.close();
          }
          File client_cert_file = LittleFS.open(MQTT_CLIENT_CERT_FILENAME, "r");
          if (client_cert_file) {
            ESP_LOGD(TAG, "Loading MQTT client cert from FS (%s)", MQTT_CLIENT_CERT_FILENAME);
            ((WiFiClientSecure*)testWifiClient)->loadCertificate(client_cert_file, client_cert_file.size());
            client_cert_file.close();
          }
        } else {
          testWifiClient = new WiFiClient();
        }
        PubSubClient* testMqttClient = new PubSubClient(*testWifiClient);
        testMqttClient->setServer(mqttConfig.mqttHost, mqttConfig.mqttServerPort);

        sprintf(buf, "CO2Monitor-%u-%s", config.deviceId, WifiManager::getMac().c_str());
        mqttTestSuccess = testMqttClient->connect(buf, mqttConfig.mqttUsername, mqttConfig.mqttPassword);
        if (mqttTestSuccess) {
          ESP_LOGD(TAG, "Test MQTT connected");
          sprintf(buf, "%s/%u/up/status", mqttConfig.mqttTopic, config.deviceId);
          char msg[256];
          doc.clear();
          doc["test"] = true;
          if (serializeJson(doc, msg) == 0) {
            ESP_LOGW(TAG, "Failed to serialise payload");
          } else {
            mqttTestSuccess = testMqttClient->publish(buf, msg);
          }
          if (!mqttTestSuccess) ESP_LOGE(TAG, "publish connect msg failed!");
        }
        if (!mqttTestSuccess) ESP_LOGE(TAG, "connecting using new mqtt settings failed!");
        if (mqttTestSuccess) {
          config = mqttConfig;
          rebootRequired = true;
        }
      }
      if (saveConfiguration(config) && rebootRequired) {
        sprintf(buf, "%s/%u/up/status", config.mqttTopic, config.deviceId);
        publishStatusMsgInternal("configuration updated, rebooting shortly");
        delay(1000);
        esp_restart();
      }
      model->configurationChanged();
    } else if (strncmp(buf, "resetWifi", strlen(buf)) == 0) {
      WifiManager::resetSettings();
    } else if (strncmp(buf, "ota", strlen(buf)) == 0) {
      OTA::checkForUpdate();
    } else if (strncmp(buf, "forceota", strlen(buf)) == 0) {
      OTA::forceUpdate(msg);
    } else if (strncmp(buf, "reboot", strlen(buf)) == 0) {
      esp_restart();
    }
  }

  void reconnect() {
    if (!WiFi.isConnected() || mqtt_client->connected()) return;
    if (millis() - lastReconnectAttempt < 60000) return;
    char topic[256];
    sprintf(topic, "CO2Monitor-%u-%s", config.deviceId, WifiManager::getMac().c_str());
    lastReconnectAttempt = millis();
    ESP_LOGD(TAG, "Attempting MQTT connection...");
    connectionAttempts++;
    if (mqtt_client->connect(topic, config.mqttUsername, config.mqttPassword)) {
      ESP_LOGD(TAG, "MQTT connected");
      sprintf(topic, "%s/%u/down/#", config.mqttTopic, config.deviceId);
      mqtt_client->subscribe(topic);
      sprintf(topic, "%s/down/#", config.mqttTopic);
      mqtt_client->subscribe(topic);
      sprintf(topic, "%s/%u/up/status", config.mqttTopic, config.deviceId);
      char msg[256];
      doc.clear();
      doc["online"] = true;
      doc["connectionAttempts"] = connectionAttempts;
      if (serializeJson(doc, msg) == 0) {
        ESP_LOGW(TAG, "Failed to serialise payload");
        return;
      }
      if (mqtt_client->publish(topic, msg))
        connectionAttempts = 0;
      else
        ESP_LOGE(TAG, "publish connect msg failed!");
    } else {
      ESP_LOGW(TAG, "MQTT connection failed, rc=%i", mqtt_client->state());
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  void setupMqtt(
    Model* _model,
    calibrateCo2SensorCallback_t _calibrateCo2SensorCallback,
    setTemperatureOffsetCallback_t _setTemperatureOffsetCallback,
    getTemperatureOffsetCallback_t _getTemperatureOffsetCallback,
    getSPS30AutoCleanIntervalCallback_t _getSPS30AutoCleanIntervalCallback,
    setSPS30AutoCleanIntervalCallback_t _setSPS30AutoCleanIntervalCallback,
    cleanSPS30Callback_t _cleanSPS30Callback,
    getSPS30StatusCallback_t _getSPS30StatusCallback
  ) {
    mqttQueue = xQueueCreate(2, sizeof(struct MqttMessage));
    if (mqttQueue == NULL) {
      ESP_LOGE(TAG, "Queue creation failed!");
    }

    model = _model;
    calibrateCo2SensorCallback = _calibrateCo2SensorCallback;
    setTemperatureOffsetCallback = _setTemperatureOffsetCallback;
    getTemperatureOffsetCallback = _getTemperatureOffsetCallback;
    getSPS30AutoCleanIntervalCallback = _getSPS30AutoCleanIntervalCallback;
    setSPS30AutoCleanIntervalCallback = _setSPS30AutoCleanIntervalCallback;
    cleanSPS30Callback = _cleanSPS30Callback;
    getSPS30StatusCallback = _getSPS30StatusCallback;

    if (config.mqttUseTls) {
      wifiClient = new WiFiClientSecure();
      if (config.mqttInsecure) {
        ((WiFiClientSecure*)wifiClient)->setInsecure();
      }
      File root_ca_file = LittleFS.open(MQTT_ROOT_CA_FILENAME, "r");
      if (root_ca_file) {
        ESP_LOGD(TAG, "Loading MQTT root ca from FS (%s)", MQTT_ROOT_CA_FILENAME);
        ((WiFiClientSecure*)wifiClient)->loadCACert(root_ca_file, root_ca_file.size());
        root_ca_file.close();
      }
      File client_key_file = LittleFS.open(MQTT_CLIENT_KEY_FILENAME, "r");
      if (client_key_file) {
        ESP_LOGD(TAG, "Loading MQTT client key from FS (%s)", MQTT_CLIENT_KEY_FILENAME);
        ((WiFiClientSecure*)wifiClient)->loadPrivateKey(client_key_file, client_key_file.size());
        client_key_file.close();
      }
      File client_cert_file = LittleFS.open(MQTT_CLIENT_CERT_FILENAME, "r");
      if (client_cert_file) {
        ESP_LOGD(TAG, "Loading MQTT client cert from FS (%s)", MQTT_CLIENT_CERT_FILENAME);
        ((WiFiClientSecure*)wifiClient)->loadCertificate(client_cert_file, client_cert_file.size());
        client_cert_file.close();
      }
    } else {
      wifiClient = new WiFiClient();
    }

    mqtt_client = new PubSubClient(*wifiClient);
    mqtt_client->setServer(config.mqttHost, config.mqttServerPort);
    mqtt_client->setCallback(callback);
    if (!mqtt_client->setBufferSize(CONFIG_SIZE)) ESP_LOGE(TAG, "mqtt_client->setBufferSize failed!");
  }

  void mqttLoop(void* pvParameters) {
    _ASSERT((uint32_t)pvParameters == 1);
    lastReconnectAttempt = millis() - 60000;
    BaseType_t notified;
    MqttMessage msg;
    while (1) {
      notified = xQueueReceive(mqttQueue, &msg, pdMS_TO_TICKS(100));
      if (notified == pdPASS) {
        if (msg.cmd == X_CMD_PUBLISH_CONFIGURATION) {
          publishConfigurationInternal();
        } else if (msg.cmd == X_CMD_PUBLISH_SENSORS) {
          publishSensorsInternal(msg.mask);
        } else if (msg.cmd == X_CMD_PUBLISH_STATUS_MSG) {
          publishStatusMsgInternal(msg.statusMessage);
        }
      }
      if (!mqtt_client->connected()) {
        reconnect();
      }
      mqtt_client->loop();
      vTaskDelay(pdMS_TO_TICKS(50));
    }
    vTaskDelete(NULL);
  }

}
