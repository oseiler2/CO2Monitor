#include <mqtt.h>
#include <Arduino.h>
#include <config.h>

#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <i2c.h>
#include <configManager.h>
#include <model.h>
#include <wifiManager.h>
#include <ota.h>

#include <ArduinoJson.h>

namespace mqtt {

  const uint32_t X_CMD_PUBLISH_SENSORS = bit(1);
  const uint32_t X_CMD_PUBLISH_CONFIGURATION = bit(2);

  TaskHandle_t mqttTask;

  WiFiClient wifiClient;
  PubSubClient mqtt_client(wifiClient);
  Model* model;

  calibrateCo2SensorCallback_t calibrateCo2SensorCallback;
  setTemperatureOffsetCallback_t setTemperatureOffsetCallback;

  void publishSensors() {
    xTaskNotify(mqttTask, X_CMD_PUBLISH_SENSORS, eSetBits);
  }

  void publishSensorsInteral() {
    char buf[256];
    char msg[256];
    sprintf(msg, "{\"co2\":%u,\"temperature\":\"%.1f\",\"humidity\":\"%.1f\"}", model->getCo2(), model->getTemperature(), model->getHumidity());
    sprintf(buf, "%s/%u/up/sensors", config.mqttTopic, config.deviceId);
    ESP_LOGI(TAG, "Publishing sensor values: %s", msg);
    mqtt_client.publish(buf, msg);
  }

  void publishConfiguration() {
    xTaskNotify(mqttTask, X_CMD_PUBLISH_CONFIGURATION, eSetBits);
  }

  void publishConfigurationInternal() {
    char buf[256];
    char msg[384];
    sprintf(msg, "{\"appVersion\":%u,\"altitude\":%u,\"yellowThreshold\":%u,\"redThreshold\":%u,\"darkRedThreshold\":%u,%s%s%s%s\"ledPwm\":%u,\"mac\":\"%x\",\"ip\":\"%s\"}",
      APP_VERSION,
      config.altitude,
      config.yellowThreshold,
      config.redThreshold,
      config.darkRedThreshold,
      I2C::scd30Present() ? "\"scd30\":true," : "",
      I2C::scd40Present() ? "\"scd40\":true," : "",
      I2C::sps30Present() ? "\"sps30\":true," : "",
      I2C::bme680Present() ? "\"bme680\":true," : "",
      I2C::lcdPresent() ? "\"lcd\":true," : "",
      config.ledPwm,
      (uint32_t)ESP.getEfuseMac(),
      WiFi.localIP().toString().c_str());
    sprintf(buf, "%s/%u/up/config", config.mqttTopic, config.deviceId);
    ESP_LOGI(TAG, "Publishing configuration: %s", msg);
    mqtt_client.publish(buf, msg);
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
      if (-10.0 < tempOffset && tempOffset <= 10.0) {
        setTemperatureOffsetCallback(tempOffset);
      }
    } else if (strncmp(buf, "getConfig", strlen(buf)) == 0) {
      publishConfiguration();
    } else if (strncmp(buf, "setConfig", strlen(buf)) == 0) {
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, msg);
      if (error) {
        ESP_LOGW(TAG, "Failed to parse message: %s", error.f_str());
        return;
      }
      if (doc["altitude"].as<int>()) config.altitude = doc["altitude"];
      if (doc["yellowThreshold"].as<int>()) config.yellowThreshold = doc["yellowThreshold"];
      if (doc["redThreshold"].as<int>()) config.redThreshold = doc["redThreshold"];
      if (doc["darkRedThreshold"].as<uint16_t>()) config.darkRedThreshold = doc["darkRedThreshold"];
      if (doc["ledPwm"].as<uint8_t>()) config.ledPwm = doc["ledPwm"];
      saveConfiguration(config);
    } else if (strncmp(buf, "resetWifi", strlen(buf)) == 0) {
      WifiManager::resetSettings();
    } else if (strncmp(buf, "ota", strlen(buf)) == 0) {
      OTA::checkForUpdate();
    } else if (strncmp(buf, "reboot", strlen(buf)) == 0) {
      esp_restart();
    }
  }

  void reconnect() {
    char buf[256];
    sprintf(buf, "CO2Monitor-%u-%x", config.deviceId, ESP.getEfuseMac());
    while (!WiFi.isConnected()) { vTaskDelay(pdMS_TO_TICKS(100)); }
    while (!mqtt_client.connected()) {
      ESP_LOGD(TAG, "Attempting MQTT connection...");
      if (mqtt_client.connect(buf, config.mqttUsername, config.mqttPassword)) {
        ESP_LOGD(TAG, "MQTT connected");
        sprintf(buf, "%s/%u/down/#", config.mqttTopic, config.deviceId);
        mqtt_client.subscribe(buf);
        sprintf(buf, "%s/down/#", config.mqttTopic);
        mqtt_client.subscribe(buf);
        sprintf(buf, "%s/%u/up/status", config.mqttTopic, config.deviceId);
        mqtt_client.publish(buf, "{\"online\":true}");
        vTaskDelay(pdMS_TO_TICKS(1000));
      } else {
        ESP_LOGW(TAG, "MQTT connection failed, rc=%i", mqtt_client.state());
        vTaskDelay(pdMS_TO_TICKS(10000));
      }
    }
  }

  void setupMqtt(Model* _model, calibrateCo2SensorCallback_t _calibrateCo2SensorCallback, setTemperatureOffsetCallback_t _setTemperatureOffsetCallback) {
    model = _model;
    calibrateCo2SensorCallback = _calibrateCo2SensorCallback;
    setTemperatureOffsetCallback = _setTemperatureOffsetCallback;
    mqtt_client.setServer(config.mqttHost, config.mqttServerPort);
    mqtt_client.setCallback(callback);
  }

  void mqttLoop(void* pvParameters) {
    _ASSERT((uint32_t)pvParameters == 1);
    uint32_t taskNotification;
    BaseType_t notified;
    while (1) {
      notified = xTaskNotifyWait(0x00,     // Don't clear any bits on entry
        ULONG_MAX,                         // Clear all bits on exit
        &taskNotification,                 // Receives the notification value
        pdMS_TO_TICKS(100));
      if (notified == pdPASS) {
        if (taskNotification & X_CMD_PUBLISH_CONFIGURATION) {
          taskNotification &= ~X_CMD_PUBLISH_CONFIGURATION;
          publishConfigurationInternal();
        }
        if (taskNotification & X_CMD_PUBLISH_SENSORS) {
          taskNotification &= ~X_CMD_PUBLISH_SENSORS;
          publishSensorsInteral();
        }
      }
      if (!mqtt_client.connected()) {
        reconnect();
      }
      mqtt_client.loop();
    }
    vTaskDelete(NULL);
  }

}