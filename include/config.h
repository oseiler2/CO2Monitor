#ifndef _CONFIG_H
#define _CONFIG_H

#define APP_VERSION           1
#define OTA_HOST              "host"
#define OTA_URL               "/co2monitor/firmware.json"
#define OTA_APP               "co2monitor"
//#define OTA_POLL

#define TRIGGER_PIN           0
#define LED_PIN               2

#define SCD30_RDY_PIN         35

#define I2C_CLK 50000UL   // SCD30 recommendation of 50kHz

static const char* CONFIG_FILENAME = "/config.json";

typedef void (*updateMessageCallback_t)(char const*);
typedef void (*setPriorityMessageCallback_t)(char const*);
typedef void (*clearPriorityMessageCallback_t)(void);

#endif
