#ifndef _CONFIG_H
#define _CONFIG_H

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
#define BTN_1                  0
#define SDA_PIN               14
#define SCL_PIN               21
#define SCD30_RDY_PIN         -1

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

#endif
