#include <Arduino.h>
#include <config.h>
#include <configManager.h>

#include <FS.h>
#include <LittleFS.h>
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
  "mqttUseTls": false,
  "mqttInsecure": false,
  "mqttServerPort": 65535,
  "altitude": 12345,
  "co2YellowThreshold": 800,
  "co2RedThreshold": 1000,
  "co2DarkRedThreshold": 2000,
  "iaqYellowThreshold": 100,
  "iaqRedThreshold": 200,
  "iaqDarkRedThreshold": 300,
  "brightness": 255,
  "ssd1306Rows": 64,
  "greenLed": 27,
  "yellowLed": 26,
  "redLed": 25,
  "neopixelData": 16,
  "neopixelNumber": 3,
  "featherMatrixData": 27,
  "featherMatrixClock": 13,
  "hub75R1": 15,
  "hub75G1": 2,
  "hub75B1": 4,
  "hub75R2": 16,
  "hub75G2": 12,
  "hub75B2": 17,
  "hub75ChA": 5,
  "hub75ChB": 18,
  "hub75ChC": 19,
  "hub75ChD": 14,
  "hub75Clk": 27,
  "hub75Lat": 26,
  "hub75Oe": 25
}
*/

void setupConfigManager() {
  if (!LittleFS.begin(true)) {
    ESP_LOGW(TAG, "LittleFS failed! Already tried formatting.");
    if (!LittleFS.begin()) {
      delay(100);
      ESP_LOGW(TAG, "LittleFS failed second time!");
    }
  }
}

#define DEFAULT_MQTT_TOPIC      "co2monitor"
#define DEFAULT_MQTT_HOST        "127.0.0.1"
#define DEFAULT_MQTT_PORT               1883
#define DEFAULT_MQTT_USERNAME   "co2monitor"
#define DEFAULT_MQTT_PASSWORD   "co2monitor"
#define DEFAULT_MQTT_USE_TLS           false
#define DEFAULT_MQTT_INSECURE          false
#define DEFAULT_DEVICE_ID                  0
#define DEFAULT_ALTITUDE                   5
#define DEFAULT_CO2_YELLOW_THRESHOLD     700
#define DEFAULT_CO2_RED_THRESHOLD        900
#define DEFAULT_CO2_DARK_RED_THRESHOLD  1200
#define DEFAULT_IAQ_YELLOW_THRESHOLD     100
#define DEFAULT_IAQ_RED_THRESHOLD        200
#define DEFAULT_IAQ_DARK_RED_THRESHOLD   300
#define DEFAULT_BRIGHTNESS               255
#define DEFAULT_SSD1306_ROWS              64
//27
#define DEFAULT_GREEN_LED                 0
#define DEFAULT_YELLOW_LED                26
#define DEFAULT_RED_LED                   25
// 16
#define DEFAULT_NEOPIXEL_DATA              0
#define DEFAULT_NEOPIXEL_NUMBER            3
// 27
#define DEFAULT_FEATHER_MATRIX_DATA        0
#define DEFAULT_FEATHER_MATRIX_CLK        13
// 15
#define DEFAULT_HUB75_R1                   0
#define DEFAULT_HUB75_G1                   2
#define DEFAULT_HUB75_B1                   4
#define DEFAULT_HUB75_R2                  16
#define DEFAULT_HUB75_G2                  12
#define DEFAULT_HUB75_B2                  17
#define DEFAULT_HUB75_CH_A                 5
#define DEFAULT_HUB75_CH_B                18
#define DEFAULT_HUB75_CH_C                19
#define DEFAULT_HUB75_CH_D                14
#define DEFAULT_HUB75_CLK                 27
#define DEFAULT_HUB75_LAT                 26
#define DEFAULT_HUB75_OE                  25

void getDefaultConfiguration(Config& config) {
  config.deviceId = DEFAULT_DEVICE_ID;
  strlcpy(config.mqttTopic, DEFAULT_MQTT_TOPIC, sizeof(DEFAULT_MQTT_TOPIC));
  strlcpy(config.mqttUsername, DEFAULT_MQTT_USERNAME, sizeof(DEFAULT_MQTT_USERNAME));
  strlcpy(config.mqttPassword, DEFAULT_MQTT_PASSWORD, sizeof(DEFAULT_MQTT_PASSWORD));
  strlcpy(config.mqttHost, DEFAULT_MQTT_HOST, sizeof(DEFAULT_MQTT_HOST));
  config.mqttUseTls = DEFAULT_MQTT_USE_TLS;
  config.mqttInsecure = DEFAULT_MQTT_INSECURE;
  config.mqttServerPort = DEFAULT_MQTT_PORT;
  config.altitude = DEFAULT_ALTITUDE;
  config.co2YellowThreshold = DEFAULT_CO2_YELLOW_THRESHOLD;
  config.co2RedThreshold = DEFAULT_CO2_RED_THRESHOLD;
  config.co2DarkRedThreshold = DEFAULT_CO2_DARK_RED_THRESHOLD;
  config.iaqYellowThreshold = DEFAULT_IAQ_YELLOW_THRESHOLD;
  config.iaqRedThreshold = DEFAULT_IAQ_RED_THRESHOLD;
  config.iaqDarkRedThreshold = DEFAULT_IAQ_DARK_RED_THRESHOLD;
  config.brightness = DEFAULT_BRIGHTNESS;
  config.ssd1306Rows = DEFAULT_SSD1306_ROWS;
  config.greenLed = DEFAULT_GREEN_LED;
  config.yellowLed = DEFAULT_YELLOW_LED;
  config.redLed = DEFAULT_RED_LED;
  config.neopixelData = DEFAULT_NEOPIXEL_DATA;
  config.neopixelNumber = DEFAULT_NEOPIXEL_NUMBER;
  config.featherMatrixData = DEFAULT_FEATHER_MATRIX_DATA;
  config.featherMatrixClock = DEFAULT_FEATHER_MATRIX_CLK;
  config.hub75R1 = DEFAULT_HUB75_R1;
  config.hub75G1 = DEFAULT_HUB75_G1;
  config.hub75B1 = DEFAULT_HUB75_B1;
  config.hub75R2 = DEFAULT_HUB75_R2;
  config.hub75G2 = DEFAULT_HUB75_G2;
  config.hub75B2 = DEFAULT_HUB75_B2;
  config.hub75ChA = DEFAULT_HUB75_CH_A;
  config.hub75ChB = DEFAULT_HUB75_CH_B;
  config.hub75ChC = DEFAULT_HUB75_CH_C;
  config.hub75ChD = DEFAULT_HUB75_CH_D;
  config.hub75Clk = DEFAULT_HUB75_CLK;
  config.hub75Lat = DEFAULT_HUB75_LAT;
  config.hub75Oe = DEFAULT_HUB75_OE;
}

void logConfiguration(const Config& config) {
  ESP_LOGD(TAG, "deviceId: %u", config.deviceId);
  ESP_LOGD(TAG, "mqttTopic: %s", config.mqttTopic);
  ESP_LOGD(TAG, "mqttUsername: %s", config.mqttUsername);
  ESP_LOGD(TAG, "mqttPassword: %s", config.mqttPassword);
  ESP_LOGD(TAG, "mqttHost: %s", config.mqttHost);
  ESP_LOGD(TAG, "mqttUseTls: %s", config.mqttUseTls ? "true" : "false");
  ESP_LOGD(TAG, "mqttInsecure: %s", config.mqttInsecure ? "true" : "false");
  ESP_LOGD(TAG, "mqttPort: %u", config.mqttServerPort);
  ESP_LOGD(TAG, "altitude: %u", config.altitude);
  ESP_LOGD(TAG, "co2YellowThreshold: %u", config.co2YellowThreshold);
  ESP_LOGD(TAG, "co2RedThreshold: %u", config.co2RedThreshold);
  ESP_LOGD(TAG, "co2DarkRedThreshold: %u", config.co2DarkRedThreshold);
  ESP_LOGD(TAG, "iaqYellowThreshold: %u", config.iaqYellowThreshold);
  ESP_LOGD(TAG, "iaqRedThreshold: %u", config.iaqRedThreshold);
  ESP_LOGD(TAG, "iaqDarkRedThreshold: %u", config.iaqDarkRedThreshold);
  ESP_LOGD(TAG, "brightness: %u", config.brightness);
  ESP_LOGD(TAG, "ssd1306Rows: %u", config.ssd1306Rows);
  ESP_LOGD(TAG, "greenLed: %u", config.greenLed);
  ESP_LOGD(TAG, "yellowLed: %u", config.yellowLed);
  ESP_LOGD(TAG, "redLed: %u", config.redLed);
  ESP_LOGD(TAG, "neopixelData: %u", config.neopixelData);
  ESP_LOGD(TAG, "neopixelNumber: %u", config.neopixelNumber);
  ESP_LOGD(TAG, "featherMatrixData: %u", config.featherMatrixData);
  ESP_LOGD(TAG, "featherMatrixClock: %u", config.featherMatrixClock);
  ESP_LOGD(TAG, "hub75R1: %u", config.hub75R1);
  ESP_LOGD(TAG, "hub75G1: %u", config.hub75G1);
  ESP_LOGD(TAG, "hub75B1: %u", config.hub75B1);
  ESP_LOGD(TAG, "hub75R2: %u", config.hub75R2);
  ESP_LOGD(TAG, "hub75G2: %u", config.hub75G2);
  ESP_LOGD(TAG, "hub75B2: %u", config.hub75B2);
  ESP_LOGD(TAG, "hub75ChA: %u", config.hub75ChA);
  ESP_LOGD(TAG, "hub75ChB: %u", config.hub75ChB);
  ESP_LOGD(TAG, "hub75ChC: %u", config.hub75ChC);
  ESP_LOGD(TAG, "hub75ChD: %u", config.hub75ChD);
  ESP_LOGD(TAG, "hub75Clk: %u", config.hub75Clk);
  ESP_LOGD(TAG, "hub75Lat: %u", config.hub75Lat);
  ESP_LOGD(TAG, "hub75Oe: %u", config.hub75Oe);
}

boolean loadConfiguration(Config& config) {
  File file = LittleFS.open(CONFIG_FILENAME, "r");
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
  config.deviceId = doc["deviceId"] | DEFAULT_DEVICE_ID;
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
  config.mqttServerPort = doc["mqttServerPort"] | DEFAULT_MQTT_PORT;
  config.mqttUseTls = doc["mqttUseTls"] | DEFAULT_MQTT_USE_TLS;
  config.mqttInsecure = doc["mqttInsecure"] | DEFAULT_MQTT_INSECURE;
  config.altitude = doc["altitude"] | DEFAULT_ALTITUDE;
  config.co2YellowThreshold = doc["co2YellowThreshold"] | DEFAULT_CO2_YELLOW_THRESHOLD;
  config.co2RedThreshold = doc["co2RedThreshold"] | DEFAULT_CO2_RED_THRESHOLD;
  config.co2DarkRedThreshold = doc["co2DarkRedThreshold"] | DEFAULT_CO2_DARK_RED_THRESHOLD;
  config.iaqYellowThreshold = doc["iaqYellowThreshold"] | DEFAULT_IAQ_YELLOW_THRESHOLD;
  config.iaqRedThreshold = doc["iaqRedThreshold"] | DEFAULT_IAQ_RED_THRESHOLD;
  config.iaqDarkRedThreshold = doc["iaqDarkRedThreshold"] | DEFAULT_IAQ_DARK_RED_THRESHOLD;
  config.brightness = doc["brightness"] | DEFAULT_BRIGHTNESS;
  config.ssd1306Rows = doc["ssd1306Rows"] | DEFAULT_SSD1306_ROWS;
  config.greenLed = doc["greenLed"] | DEFAULT_GREEN_LED;
  config.yellowLed = doc["yellowLed"] | DEFAULT_YELLOW_LED;
  config.redLed = doc["redLed"] | DEFAULT_RED_LED;
  config.neopixelData = doc["neopixelData"] | DEFAULT_NEOPIXEL_DATA;
  config.neopixelNumber = doc["neopixelNumber"] | DEFAULT_NEOPIXEL_NUMBER;
  config.featherMatrixData = doc["featherMatrixData"] | DEFAULT_FEATHER_MATRIX_DATA;
  config.featherMatrixClock = doc["featherMatrixClock"] | DEFAULT_FEATHER_MATRIX_CLK;
  config.hub75R1 = doc["hub75R1"] | DEFAULT_HUB75_R1;
  config.hub75G1 = doc["hub75G1"] | DEFAULT_HUB75_G1;
  config.hub75B1 = doc["hub75B1"] | DEFAULT_HUB75_B1;
  config.hub75R2 = doc["hub75R2"] | DEFAULT_HUB75_R2;
  config.hub75G2 = doc["hub75G2"] | DEFAULT_HUB75_G2;
  config.hub75B2 = doc["hub75B2"] | DEFAULT_HUB75_B2;
  config.hub75ChA = doc["hub75ChA"] | DEFAULT_HUB75_CH_A;
  config.hub75ChB = doc["hub75ChB"] | DEFAULT_HUB75_CH_B;
  config.hub75ChC = doc["hub75ChC"] | DEFAULT_HUB75_CH_C;
  config.hub75ChD = doc["hub75ChD"] | DEFAULT_HUB75_CH_D;
  config.hub75Clk = doc["hub75Clk"] | DEFAULT_HUB75_CLK;
  config.hub75Lat = doc["hub75Lat"] | DEFAULT_HUB75_LAT;
  config.hub75Oe = doc["hub75Oe"] | DEFAULT_HUB75_OE;

  file.close();
  return true;
}

boolean saveConfiguration(const Config& config) {
  ESP_LOGD(TAG, "###################### saveConfiguration");
  logConfiguration(config);
  // Delete existing file, otherwise the configuration is appended to the file
  if (LittleFS.exists(CONFIG_FILENAME)) {
    LittleFS.remove(CONFIG_FILENAME);
  }

  // Open file for writing
  File file = LittleFS.open(CONFIG_FILENAME, "w");
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
  doc["ssd1306Rows"] = config.ssd1306Rows;
  doc["greenLed"] = config.greenLed;
  doc["yellowLed"] = config.yellowLed;
  doc["redLed"] = config.redLed;
  doc["neopixelData"] = config.neopixelData;
  doc["neopixelNumber"] = config.neopixelNumber;
  doc["featherMatrixData"] = config.featherMatrixData;
  doc["featherMatrixClock"] = config.featherMatrixClock;
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
  File file = LittleFS.open(CONFIG_FILENAME, "r");
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
