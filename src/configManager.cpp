#include <Arduino.h>
#include <config.h>
#include <configManager.h>

#include <FS.h>
#include <LITTLEFS.h>
#include <ArduinoJson.h>

// Local logging tag
static const char TAG[] = __FILE__;

Config config;

// Allocate a temporary JsonDocument
// Don't forget to change the capacity to match your requirements.
// Use arduinojson.org/v6/assistant to compute the capacity.
/*
{
  "deviceId": 65535,
  "mqttTopic": "123456789112345678921",
  "mqttUsername": "123456789112345678921",
  "mqttPassword": "123456789112345678921",
  "mqttHost": "1234567891123456789212345678931",
  "mqttServerPort": 65535,
  "altitude": 12345,
  "yellowThreshold": 800,
  "redThreshold": 1000,
  "darkRedThreshold": 2000,
  "ledPwm": 100
}
*/
#define CONFIG_SIZE 768

void setupConfigManager() {
  if (!LITTLEFS.begin(true)) {
    ESP_LOGW(TAG, "LittleFS failed! Already tried formatting.");
    if (!LITTLEFS.begin()) {
      delay(100);
      ESP_LOGW(TAG, "LittleFS failed second time!");
    }
  }
}

#define DEFAULT_MQTT_TOPIC "co2monitor"
#define DEFAULT_MQTT_HOST "127.0.0.1"
#define DEFAULT_MQTT_USERNAME "co2monitor"
#define DEFAULT_MQTT_PASSWORD "co2monitor"

void getDefaultConfiguration(Config& config) {
  config.deviceId = 0;
  strlcpy(config.mqttTopic, DEFAULT_MQTT_TOPIC, sizeof(DEFAULT_MQTT_TOPIC));
  strlcpy(config.mqttUsername, DEFAULT_MQTT_USERNAME, sizeof(DEFAULT_MQTT_USERNAME));
  strlcpy(config.mqttPassword, DEFAULT_MQTT_PASSWORD, sizeof(DEFAULT_MQTT_PASSWORD));
  strlcpy(config.mqttHost, DEFAULT_MQTT_HOST, sizeof(DEFAULT_MQTT_HOST));
  config.mqttServerPort = 1883;
  config.altitude = 5;
  config.yellowThreshold = 700;
  config.redThreshold = 900;
  config.darkRedThreshold = 1200;
  config.ledPwm = 255;
}

void logConfiguration(const Config& config) {
  ESP_LOGD(TAG, "deviceId: %u", config.deviceId);
  ESP_LOGD(TAG, "mqttTopic: %s", config.mqttTopic);
  ESP_LOGD(TAG, "mqttUsername: %s", config.mqttUsername);
  ESP_LOGD(TAG, "mqttPassword: %s", config.mqttPassword);
  ESP_LOGD(TAG, "mqttHost: %s", config.mqttHost);
  ESP_LOGD(TAG, "mqttPort: %u", config.mqttServerPort);
  ESP_LOGD(TAG, "altitude: %u", config.altitude);
  ESP_LOGD(TAG, "yellowThreshold: %u", config.yellowThreshold);
  ESP_LOGD(TAG, "redThreshold: %u", config.redThreshold);
  ESP_LOGD(TAG, "darkRedThreshold: %u", config.darkRedThreshold);
  ESP_LOGD(TAG, "ledPwm: %u", config.ledPwm);
}

boolean loadConfiguration(Config& config) {
  File file = LITTLEFS.open(CONFIG_FILENAME, "r");
  if (!file) {
    ESP_LOGW(TAG, "Could not open config file");
    return false;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<CONFIG_SIZE> doc;

  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    ESP_LOGW(TAG, "Failed to parse config file: %s", error.f_str());
    return false;
  }

  // Copy values from the JsonDocument to the Config
  config.deviceId = doc["deviceId"] | 0;
  strlcpy(config.mqttTopic,
    doc["mqttTopic"] | DEFAULT_MQTT_TOPIC,
    sizeof(config.mqttTopic));
  strlcpy(config.mqttUsername,
    doc["mqttUsername"] | DEFAULT_MQTT_USERNAME,
    sizeof(config.mqttUsername));
  strlcpy(config.mqttPassword,
    doc["mqttPassword"] | DEFAULT_MQTT_PASSWORD,
    sizeof(config.mqttPassword));
  strlcpy(config.mqttHost,
    doc["mqttHost"] | DEFAULT_MQTT_HOST,
    sizeof(config.mqttHost));
  config.mqttServerPort = doc["mqttServerPort"] | 1883;
  config.altitude = doc["altitude"] | 5;
  config.yellowThreshold = doc["yellowThreshold"] | 700;
  config.redThreshold = doc["redThreshold"] | 900;
  config.darkRedThreshold = doc["darkRedThreshold"] | 1200;
  config.ledPwm = doc["ledPwm"] | 255;

  file.close();
  ESP_LOGD(TAG, "###################### loadConfiguration");
  logConfiguration(config);
  return true;
}

boolean saveConfiguration(const Config& config) {
  ESP_LOGD(TAG, "###################### saveConfiguration");
  logConfiguration(config);
  // Delete existing file, otherwise the configuration is appended to the file
  if (LITTLEFS.exists(CONFIG_FILENAME)) {
    LITTLEFS.remove(CONFIG_FILENAME);
  }

  // Open file for writing
  File file = LITTLEFS.open(CONFIG_FILENAME, "w");
  if (!file) {
    ESP_LOGW(TAG, "Could not create config file for writing");
    return false;
  }

  StaticJsonDocument<CONFIG_SIZE> doc;

  // Set the values in the document
  doc["deviceId"] = config.deviceId;
  doc["mqttTopic"] = config.mqttTopic;
  doc["mqttUsername"] = config.mqttUsername;
  doc["mqttPassword"] = config.mqttPassword;
  doc["mqttHost"] = config.mqttHost;
  doc["mqttServerPort"] = config.mqttServerPort;
  doc["altitude"] = config.altitude;
  doc["yellowThreshold"] = config.yellowThreshold;
  doc["redThreshold"] = config.redThreshold;
  doc["darkRedThreshold"] = config.darkRedThreshold;
  doc["ledPwm"] = config.ledPwm;

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    ESP_LOGW(TAG, "Failed to write to file");
    file.close();
    return false;
  }

  // Close the file
  file.close();
  ESP_LOGD(TAG, "Stored configuration successfully");
  return true;
}

// Prints the content of a file to the Serial
void printFile() {
  // Open file for reading
  File file = LITTLEFS.open(CONFIG_FILENAME, "r");
  if (!file) {
    ESP_LOGW(TAG, "Could not open config file");
    return;
  }

  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file
  file.close();
}

