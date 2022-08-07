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

#endif


#define I2C_CLK 100000UL
#define SCD30_I2C_CLK 50000UL   // SCD30 recommendation of 50kHz

static const char* CONFIG_FILENAME = "/config.json";
static const char* MQTT_ROOT_CA_FILENAME = "/mqtt_root_ca.pem";
static const char* MQTT_CLIENT_CERT_FILENAME = "/mqtt_client_cert.pem";
static const char* MQTT_CLIENT_KEY_FILENAME = "/mqtt_client_key.pem";

#define PWM_CHANNEL_LEDS        0
#define PWM_CHANNEL_BUZZER      1

#endif
