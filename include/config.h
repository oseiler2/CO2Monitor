#pragma once

#include <variant.h>
#include <Arduino.h>

#define OTA_URL               "https://otahost/co2monitor/firmware.json"
#define OTA_APP               "co2monitor"
//#define OTA_POLL

#if not defined (LED_PIN)
#define LED_PIN             (-1)
#endif

#if not defined (SCD30_RDY_PIN)
#define SCD30_RDY_PIN       (-1)
#endif

#if defined (BTN_1)
#define HAS_BTN_1            (1)
#else
#define HAS_BTN_1            (0)
#endif

#if defined (BTN_2)
#define HAS_BTN_2            (1)
#else
#define HAS_BTN_2            (0)
#endif

#if defined (BTN_3)
#define HAS_BTN_3            (1)
#else
#define HAS_BTN_3            (0)
#endif

#if defined (BTN_4)
#define HAS_BTN_4            (1)
#else
#define HAS_BTN_4            (0)
#endif

#if HAS_BTN_1 && HAS_BTN_2 && HAS_BTN_3 && HAS_BTN_4
#define HAS_MENU_BTNS        (1)
#else
#define HAS_MENU_BTNS        (0)
#endif

#if defined (VBAT_EN) && defined (VBAT_ADC)
#define HAS_BATTERY          (1)
#else
#define HAS_BATTERY          (0)
#endif

#if defined (OLED_EN)
#define HAS_OLED_EN          (1)
#else
#define HAS_OLED_EN          (0)
#endif

#if defined (BUZZER_PIN)
#define HAS_BUZZER           (1)
#else
#define HAS_BUZZER           (0)
#endif

#if defined (SD_DETECT) && defined (SD_DAT0) && defined (SD_DAT1) && defined (SD_DAT2) && defined (SD_DAT3) && defined (SD_CLK) && defined (SD_CMD)
#define HAS_SD_SLOT          (1)
#else
#define HAS_SD_SLOT          (0)
#endif

#if not defined KEEP_CAPTIVE_PORTAL_IF_NOT_CONNECTED
#define KEEP_CAPTIVE_PORTAL_IF_NOT_CONNECTED    true
#endif

#if not defined NEO_NUMBER
#define NEO_NUMBER             (3)
#endif

#if not defined LOG_TO_INTERNAL_FLASH
#define LOG_TO_INTERNAL_FLASH                 false
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
#if HAS_BUZZER  
  BuzzerMode buzzerMode;
#endif
#if HAS_BATTERY
  SleepModeOledLed sleepModeOledLed;
#endif
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
};

