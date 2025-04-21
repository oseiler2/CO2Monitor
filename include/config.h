#pragma once

#include <logging.h>
#include <sdkconfig.h>

#define OTA_URL               "https://otahost/co2monitor/firmware.json"
#define OTA_APP               "co2monitor"
//#define OTA_POLL

#if CONFIG_IDF_TARGET_ESP32

#define LED_PIN                2
#define BTN_1                  0
#define SDA_PIN              SDA
#define SCL_PIN              SCL
#define SCD30_RDY_PIN         35

#elif CONFIG_IDF_TARGET_ESP32S3

#define LED_PIN                2
#define BUZZER_PIN             1
#define SD_DETECT              3
#define SD_DAT0                5
#define SD_DAT1                4
#define SD_DAT2                8
#define SD_DAT3               18
#define SD_CLK                 6
#define SD_CMD                 7
#define VBAT_ADC               9
#define BTN_1                  0
#define BTN_2                 12
#define BTN_3                 11
#define BTN_4                 10
#define OLED_EN               13
#define NEO_DATA              17
#define XTAL_32k_1            15
#define XTAL_32k_2            16
#define VBAT_EN               46
#define SDA_PIN               14
#define SCL_PIN               21
#define SCD30_RDY_PIN         -1
#define NEO_1_EN              48
#define NEO_23_EN             47

#endif

#define I2C_CLK 100000UL
#define SCD30_I2C_CLK 50000UL   // SCD30 recommendation of 50kHz

static const char* CONFIG_FILENAME = "/config.json";
static const char* MQTT_ROOT_CA_FILENAME = "/mqtt_root_ca.pem";
static const char* MQTT_CLIENT_CERT_FILENAME = "/mqtt_client_cert.pem";
static const char* MQTT_CLIENT_KEY_FILENAME = "/mqtt_client_key.pem";
static const char* TEMP_MQTT_ROOT_CA_FILENAME = "/temp_mqtt_root_ca.pem";
static const char* ROOT_CA_FILENAME = "/root_ca.pem";

#define MQTT_QUEUE_LENGTH      25

#define PWM_CHANNEL_LEDS        0
#define PWM_CHANNEL_BUZZER      1

// ----------------------------  Config struct ------------------------------------- 
#define CONFIG_SIZE 1536

#define MQTT_USERNAME_LEN 20
#define MQTT_PASSWORD_LEN 20
#define MQTT_HOSTNAME_LEN 30
#define MQTT_TOPIC_LEN 30
#define SSID_LEN 32
#define WIFI_PASSWORD_LEN 64

typedef enum {
  BUZ_OFF = 0,
  BUZ_LVL_CHANGE,
  BUZ_ALWAYS
} BuzzerMode;

typedef enum {
  SLEEP_OLED_ON_LED_ON = 0,
  SLEEP_OLED_ON_LED_OFF,
  SLEEP_OLED_OFF_LED_ON,
  SLEEP_OLED_OFF_LED_OFF
} SleepModeOledLed;

struct Config {
  uint16_t deviceId;
  char mqttTopic[MQTT_TOPIC_LEN + 1];
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
  BuzzerMode buzzerMode;
  SleepModeOledLed sleepModeOledLed;
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

  uint8_t vBatEn = VBAT_EN;
  uint8_t vBatAdc = VBAT_ADC;
  uint8_t btn2 = BTN_2;
  uint8_t btn3 = BTN_3;
  uint8_t btn4 = BTN_4;
  uint8_t oledEn = OLED_EN;
  uint8_t buzzerPin = BUZZER_PIN;
  uint8_t sdDetect = SD_DETECT;
  uint8_t sdDat0 = SD_DAT0;
  uint8_t sdDat1 = SD_DAT1;
  uint8_t sdDat2 = SD_DAT2;
  uint8_t sdDat3 = SD_DAT3;
  uint8_t sdClk = SD_CLK;
  uint8_t sdCmd = SD_CMD;
};

