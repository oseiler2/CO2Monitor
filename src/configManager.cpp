#include <configManager.h>

#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// Local logging tag
static const char TAG[] = "ConfigManager";

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
  "co2GreenThreshold": 0,
  "co2YellowThreshold": 800,
  "co2RedThreshold": 1000,
  "co2DarkRedThreshold": 2000,
  "iagGreenThreshold": 0,
  "iaqYellowThreshold": 100,
  "iaqRedThreshold": 200,
  "iaqDarkRedThreshold": 300,
  "brightness": 255,
  "buzzerMode": 1,
  "ssd1306Rows": 64,
  "greenLed": 27,
  "yellowLed": 26,
  "redLed": 25,
  "neopixelData": 16,
  "neopixelNumber": 3,
  "neopixelMatrixData": 14,
  "featherMatrixData": 27,
  "featherMatrixClock": 13,
  "matrixColumns": 12,
  "matrixRows": 5,
  "matrixLayout": 0,
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
  "hub75Oe": 25,
  "sleepModeOledLed": 0
}
*/

#define DEFAULT_DEVICE_ID                  0
#define DEFAULT_MQTT_TOPIC      "co2monitor"
#define DEFAULT_MQTT_HOST        "127.0.0.1"
#define DEFAULT_MQTT_PORT               1883
#define DEFAULT_MQTT_USERNAME   "co2monitor"
#define DEFAULT_MQTT_PASSWORD   "co2monitor"
#define DEFAULT_MQTT_USE_TLS           false
#define DEFAULT_MQTT_INSECURE          false
#define DEFAULT_ALTITUDE                   5
#define DEFAULT_CO2_GREEN_THRESHOLD        0
#define DEFAULT_CO2_YELLOW_THRESHOLD     700
#define DEFAULT_CO2_RED_THRESHOLD        900
#define DEFAULT_CO2_DARK_RED_THRESHOLD  1200
#define DEFAULT_IAQ_GREEN_THRESHOLD        0
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
#define DEFAULT_NEOPIXEL_DATA              NEO_DATA
#define DEFAULT_NEOPIXEL_NUMBER            3
// 14
#define DEFAULT_NEOPIXEL_MATRIX_DATA       0
// 27
#define DEFAULT_FEATHER_MATRIX_DATA        0
#define DEFAULT_FEATHER_MATRIX_CLK        13
#define DEFAULT_MATRIX_COLUMNS            12
#define DEFAULT_MATRIX_ROWS                5
#define DEFAULT_MATRIX_LAYOUT              0
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
#define DEFAULT_BUZZER_MODE               BUZ_LVL_CHANGE
#define DEFAULT_SLEEP_MODE_OLED_LED       SLEEP_OLED_ON_LED_ON

std::vector<ConfigParameterBase<Config>*> configParameterVector;

const char* BUZZER_MODE_STRINGS[] = {
  "Buzzer off",
  "Buzzer when level changes",
  "Buzzer always on",
};

const char* SLEEP_MODE_OLED_LED_STRINGS[] = {
    "Display on, LED on",
    "Display on, LED off",
    "Display off, LED on",
    "Display off, LED off"
};

void setupConfigManager() {
  if (!LittleFS.begin(true)) {
    ESP_LOGW(TAG, "LittleFS failed! Already tried formatting.");
    vTaskDelay(pdMS_TO_TICKS(100));
    if (!LittleFS.begin()) {
      ESP_LOGW(TAG, "LittleFS failed second time!");
    }
  }
  //  configParameterVector.clear();
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("deviceId", "Device ID", &Config::deviceId, DEFAULT_DEVICE_ID));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("mqttTopic", "MQTT topic", (char Config::*) & Config::mqttTopic, DEFAULT_MQTT_TOPIC, MQTT_TOPIC_LEN));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("mqttUsername", "MQTT username", (char Config::*) & Config::mqttUsername, DEFAULT_MQTT_USERNAME, MQTT_USERNAME_LEN));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("mqttPassword", "MQTT password", (char Config::*) & Config::mqttPassword, DEFAULT_MQTT_PASSWORD, MQTT_PASSWORD_LEN));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("mqttHost", "MQTT host", (char Config::*) & Config::mqttHost, DEFAULT_MQTT_HOST, MQTT_HOSTNAME_LEN));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("mqttServerPort", "MQTT port", &Config::mqttServerPort, DEFAULT_MQTT_PORT));
  configParameterVector.push_back(new BooleanConfigParameter<Config>("mqttUseTls", "MQTT use TLS", &Config::mqttUseTls, DEFAULT_MQTT_USE_TLS));
  configParameterVector.push_back(new BooleanConfigParameter<Config>("mqttInsecure", "MQTT ignore certificate errors", &Config::mqttInsecure, DEFAULT_MQTT_INSECURE));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("altitude", "Altitude", &Config::altitude, DEFAULT_ALTITUDE, 0, 8000));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("co2GreenThreshold", "CO2 Green threshold ", &Config::co2GreenThreshold, DEFAULT_CO2_GREEN_THRESHOLD));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("co2YellowThreshold", "CO2 Yellow threshold ", &Config::co2YellowThreshold, DEFAULT_CO2_YELLOW_THRESHOLD));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("co2RedThreshold", "CO2 Red threshold", &Config::co2RedThreshold, DEFAULT_CO2_RED_THRESHOLD));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("co2DarkRedThreshold", "CO2 Dark red threshold", &Config::co2DarkRedThreshold, DEFAULT_CO2_DARK_RED_THRESHOLD));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("iaqGreenThreshold", "IAQ Green threshold ", &Config::iaqGreenThreshold, DEFAULT_IAQ_GREEN_THRESHOLD));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("iaqYellowThreshold", "IAQ Yellow threshold ", &Config::iaqYellowThreshold, DEFAULT_IAQ_YELLOW_THRESHOLD));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("iaqRedThreshold", "IAQ Red threshold", &Config::iaqRedThreshold, DEFAULT_IAQ_RED_THRESHOLD));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("iaqDarkRedThreshold", "IAQ Dark red threshold", &Config::iaqDarkRedThreshold, DEFAULT_IAQ_DARK_RED_THRESHOLD));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("brightness", "LED brightness pwm", &Config::brightness, DEFAULT_BRIGHTNESS));
  configParameterVector.push_back(new EnumConfigParameter<Config, uint8_t, BuzzerMode>("buzzerMode", "Buzzer mode", &Config::buzzerMode, DEFAULT_BUZZER_MODE, BUZZER_MODE_STRINGS, BUZ_OFF, BUZ_ALWAYS));
  configParameterVector.push_back(new EnumConfigParameter<Config, uint8_t, SleepModeOledLed>("sleepModeOledLed", "Display/LEDs sleep mode", &Config::sleepModeOledLed, SLEEP_OLED_ON_LED_ON, SLEEP_MODE_OLED_LED_STRINGS, SLEEP_OLED_ON_LED_ON, SLEEP_OLED_OFF_LED_OFF));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("ssd1306Rows", "SSD1306 rows", &Config::ssd1306Rows, DEFAULT_SSD1306_ROWS, 32, 64, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("greenLed", "Green Led pin", &Config::greenLed, DEFAULT_GREEN_LED, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("yellowLed", "Yellow Led pin", &Config::yellowLed, DEFAULT_YELLOW_LED, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("redLed", "Red Led pin", &Config::redLed, DEFAULT_RED_LED, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("neopixelData", "Neopixel data pin", &Config::neopixelData, DEFAULT_NEOPIXEL_DATA, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("neopixelNumber", "Number of Neopixels", &Config::neopixelNumber, DEFAULT_NEOPIXEL_NUMBER, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("neopixelMatrixData", "Neopixel matrix data pin", &Config::neopixelMatrixData, DEFAULT_NEOPIXEL_MATRIX_DATA, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("featherMatrixData", "Feather matrix data pin", &Config::featherMatrixData, DEFAULT_FEATHER_MATRIX_DATA, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("featherMatrixClock", "Feather matrix clock pin", &Config::featherMatrixClock, DEFAULT_FEATHER_MATRIX_CLK, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("matrixColumns", "Matrix columns", &Config::matrixColumns, DEFAULT_MATRIX_COLUMNS, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("matrixRows", "Matrix rows", &Config::matrixRows, DEFAULT_MATRIX_ROWS, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("matrixLayout", "Matrix layout", &Config::matrixLayout, DEFAULT_MATRIX_LAYOUT, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("hub75R1", "Hub75 R1 pin", &Config::hub75R1, DEFAULT_HUB75_R1, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("hub75G1", "Hub75 G1 pin", &Config::hub75G1, DEFAULT_HUB75_G1, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("hub75B1", "Hub75 B1 pin", &Config::hub75B1, DEFAULT_HUB75_B1, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("hub75R2", "Hub75 R2 pin", &Config::hub75R2, DEFAULT_HUB75_R2, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("hub75G2", "Hub75 G2 pin", &Config::hub75G2, DEFAULT_HUB75_G2, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("hub75B2", "Hub75 B2 pin", &Config::hub75B2, DEFAULT_HUB75_B2, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("hub75ChA", "Hub75 Channel A pin", &Config::hub75ChA, DEFAULT_HUB75_CH_A, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("hub75ChB", "Hub75 Channel B pin", &Config::hub75ChB, DEFAULT_HUB75_CH_B, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("hub75ChC", "Hub75 Channel C pin", &Config::hub75ChC, DEFAULT_HUB75_CH_C, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("hub75ChD", "Hub75 Channel D pin", &Config::hub75ChD, DEFAULT_HUB75_CH_D, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("hub75Clk", "Hub75 Clk pin", &Config::hub75Clk, DEFAULT_HUB75_CLK, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("hub75Lat", "Hub75 Lat pin", &Config::hub75Lat, DEFAULT_HUB75_LAT, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("hub75Oe", "Hub75 Oe pin", &Config::hub75Oe, DEFAULT_HUB75_OE, true));
}

std::vector<ConfigParameterBase<Config>*> getConfigParameters() {
  return configParameterVector;
}

void getDefaultConfiguration(Config& _config) {
  for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
    configParameter->setToDefault(_config);
  }
}

void logConfiguration(const Config _config) {
  for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
    ESP_LOGD(TAG, "%s: %s", configParameter->getId(), configParameter->toString(_config).c_str());
  }
}

boolean loadConfiguration(Config& _config) {
  File file = LittleFS.open(CONFIG_FILENAME, FILE_READ);
  if (!file) {
    ESP_LOGW(TAG, "Could not open config file");
    return false;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  DynamicJsonDocument* doc = new DynamicJsonDocument(CONFIG_SIZE);

  DeserializationError error = deserializeJson(*doc, file);
  if (error) {
    ESP_LOGW(TAG, "Failed to parse config file: %s", error.f_str());
    file.close();
    return false;
  }

  for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
    configParameter->fromJson(_config, doc, true);
  }

  file.close();
  return true;
}

boolean saveConfiguration(const Config _config) {
  ESP_LOGD(TAG, "###################### saveConfiguration");
  logConfiguration(_config);
  // Delete existing file, otherwise the configuration is appended to the file
  if (LittleFS.exists(CONFIG_FILENAME)) {
    LittleFS.remove(CONFIG_FILENAME);
  }

  // Open file for writing
  File file = LittleFS.open(CONFIG_FILENAME, FILE_WRITE);
  if (!file) {
    ESP_LOGW(TAG, "Could not create config file for writing");
    return false;
  }

  DynamicJsonDocument* doc = new DynamicJsonDocument(CONFIG_SIZE);
  for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
    configParameter->toJson(_config, doc);
  }

  // Serialize JSON to file
  if (serializeJson(*doc, file) == 0) {
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
  File file = LittleFS.open(CONFIG_FILENAME, FILE_READ);
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

BuzzerMode getBuzzerModeFromUint(uint8_t buzzerMode) {
  switch (buzzerMode) {
    case 1: return BUZ_LVL_CHANGE;
    case 2: return BUZ_ALWAYS;
    default: return BUZ_OFF;
  }
}

SleepModeOledLed getSleepModeOledLedFromUint(uint8_t mode) {
  switch (mode) {
    case 1: return SLEEP_OLED_ON_LED_OFF;
    case 2: return SLEEP_OLED_OFF_LED_ON;
    case 3: return SLEEP_OLED_OFF_LED_OFF;
    default: return SLEEP_OLED_ON_LED_ON;
  }
}
