#ifndef _CONFIG_MANAGER_H
#define _CONFIG_MANAGER_H

#include <Arduino.h>
#include <config.h>

#define CONFIG_SIZE 1280

#define MQTT_USERNAME_LEN 20
#define MQTT_PASSWORD_LEN 20
#define MQTT_HOSTNAME_LEN 30
#define MQTT_TOPIC_ID_LEN 30
#define SSID_LEN 32
#define WIFI_PASSWORD_LEN 64

struct Config {
  uint16_t deviceId;
  char mqttTopic[MQTT_TOPIC_ID_LEN + 1];
  char mqttUsername[MQTT_USERNAME_LEN + 1];
  char mqttPassword[MQTT_PASSWORD_LEN + 1];
  char mqttHost[MQTT_HOSTNAME_LEN + 1];
  bool mqttUseTls;
  bool mqttInsecure;
  uint16_t mqttServerPort;
  uint16_t altitude;
  uint16_t co2GreenThreshold;
  uint16_t co2YellowThreshold;
  uint16_t co2RedThreshold;
  uint16_t co2DarkRedThreshold;
  uint16_t iaqGreenThreshold;
  uint16_t iaqYellowThreshold;
  uint16_t iaqRedThreshold;
  uint16_t iaqDarkRedThreshold;
  uint8_t brightness;
  uint8_t ssd1306Rows;
  uint8_t greenLed;
  uint8_t yellowLed;
  uint8_t redLed;
  uint8_t neopixelData;
  uint8_t neopixelNumber;
  uint8_t neopixelMatrixData;
  uint8_t featherMatrixData;
  uint8_t featherMatrixClock;
  uint8_t matrixColumns;
  uint8_t matrixRows;
  uint8_t matrixLayout;
  uint8_t hub75R1;
  uint8_t hub75G1;
  uint8_t hub75B1;
  uint8_t hub75R2;
  uint8_t hub75G2;
  uint8_t hub75B2;
  uint8_t hub75ChA;
  uint8_t hub75ChB;
  uint8_t hub75ChC;
  uint8_t hub75ChD;
  uint8_t hub75Clk;
  uint8_t hub75Lat;
  uint8_t hub75Oe;
};

static uint16_t Config::* deviceIdPtr = &Config::deviceId;
static char Config::* mqttTopicPtr = (char Config::*) & Config::mqttTopic;
static char Config::* mqttUsernamePtr = (char Config::*) & Config::mqttUsername;
static char Config::* mqttPasswordPtr = (char Config::*) & Config::mqttPassword;
static char Config::* mqttHostPtr = (char Config::*) & Config::mqttHost;
static bool Config::* mqttUseTlsPtr = &Config::mqttUseTls;
static bool Config::* mqttInsecurePtr = &Config::mqttInsecure;
static uint16_t Config::* mqttServerPortPtr = &Config::mqttServerPort;
static uint16_t Config::* altitudePtr = &Config::altitude;
static uint16_t Config::* co2GreenThresholdPtr = &Config::co2GreenThreshold;
static uint16_t Config::* co2YellowThresholdPtr = &Config::co2YellowThreshold;
static uint16_t Config::* co2RedThresholdPtr = &Config::co2RedThreshold;
static uint16_t Config::* co2DarkRedThresholdPtr = &Config::co2DarkRedThreshold;
static uint16_t Config::* iaqGreenThresholdPtr = &Config::iaqGreenThreshold;
static uint16_t Config::* iaqYellowThresholdPtr = &Config::iaqYellowThreshold;
static uint16_t Config::* iaqRedThresholdPtr = &Config::iaqRedThreshold;
static uint16_t Config::* iaqDarkRedThresholdPtr = &Config::iaqDarkRedThreshold;
static uint8_t Config::* brightnessPtr = &Config::brightness;
static uint8_t Config::* ssd1306RowsPtr = &Config::ssd1306Rows;
static uint8_t Config::* greenLedPtr = &Config::greenLed;
static uint8_t Config::* yellowLedPtr = &Config::yellowLed;
static uint8_t Config::* redLedPtr = &Config::redLed;
static uint8_t Config::* neopixelDataPtr = &Config::neopixelData;
static uint8_t Config::* neopixelNumberPtr = &Config::neopixelNumber;
static uint8_t Config::* neopixelMatrixDataPtr = &Config::neopixelMatrixData;
static uint8_t Config::* featherMatrixDataPtr = &Config::featherMatrixData;
static uint8_t Config::* featherMatrixClockPtr = &Config::featherMatrixClock;
static uint8_t Config::* matrixColumnsPtr = &Config::matrixColumns;
static uint8_t Config::* matrixRowsPtr = &Config::matrixRows;
static uint8_t Config::* matrixLayoutPtr = &Config::matrixLayout;
static uint8_t Config::* hub75R1Ptr = &Config::hub75R1;
static uint8_t Config::* hub75G1Ptr = &Config::hub75G1;
static uint8_t Config::* hub75B1Ptr = &Config::hub75B1;
static uint8_t Config::* hub75R2Ptr = &Config::hub75R2;
static uint8_t Config::* hub75G2Ptr = &Config::hub75G2;
static uint8_t Config::* hub75B2Ptr = &Config::hub75B2;
static uint8_t Config::* hub75ChAPtr = &Config::hub75ChA;
static uint8_t Config::* hub75ChBPtr = &Config::hub75ChB;
static uint8_t Config::* hub75ChCPtr = &Config::hub75ChC;
static uint8_t Config::* hub75ChDPtr = &Config::hub75ChD;
static uint8_t Config::* hub75ClkPtr = &Config::hub75Clk;
static uint8_t Config::* hub75LatPtr = &Config::hub75Lat;
static uint8_t Config::* hub75OePtr = &Config::hub75Oe;

void setupConfigManager();
void getDefaultConfiguration(Config& config);
boolean loadConfiguration(Config& config);
boolean saveConfiguration(const Config& config);
void logConfiguration(const Config& config);
void printFile();

extern Config config;

#endif