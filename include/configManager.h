#ifndef _CONFIG_MANAGER_H
#define _CONFIG_MANAGER_H

#define CONFIG_SIZE 1280

#define MQTT_USERNAME_LEN 20
#define MQTT_PASSWORD_LEN 20
#define MQTT_HOSTNAME_LEN 30
#define MQTT_TOPIC_ID_LEN 30
#define SSID_LEN 20
#define PASSWORD_LEN 20

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
  uint16_t co2YellowThreshold;
  uint16_t co2RedThreshold;
  uint16_t co2DarkRedThreshold;
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
  uint8_t featherMatrixData;
  uint8_t featherMatrixClock;
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

void setupConfigManager();
void getDefaultConfiguration(Config& config);
boolean loadConfiguration(Config& config);
boolean saveConfiguration(const Config& config);
void logConfiguration(const Config& config);
void printFile();

extern Config config;

#endif