#ifndef _CONFIG_H
#define _CONFIG_H

#define OTA_URL               "https://otahost/co2monitor/firmware.json"
#define OTA_APP               "co2monitor"
//#define OTA_POLL

#define TRIGGER_PIN           0
#define LED_PIN               2

#define SCD30_RDY_PIN         35

#define I2C_CLK 50000UL   // SCD30 recommendation of 50kHz

static const char* CONFIG_FILENAME = "/config.json";
static const char* MQTT_ROOT_CA_FILENAME = "/mqtt_root_ca.pem";
static const char* MQTT_SERVER_CERT_FILENAME = "/mqtt_server_cert.pem";

#endif
