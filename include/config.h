#ifndef _CONFIG_H
#define _CONFIG_H

#define OTA_URL               "https://otahost/co2monitor/firmware.json"
#define OTA_APP               "co2monitor"
//#define OTA_POLL

#define TRIGGER_PIN           0
#define LED_PIN               2

#define SCD30_RDY_PIN         35

#define I2C_CLK 100000UL
#define SCD30_I2C_CLK 50000UL   // SCD30 recommendation of 50kHz

static const char* CONFIG_FILENAME = "/config.json";
static const char* MQTT_ROOT_CA_FILENAME = "/mqtt_root_ca.pem";
// Certs and keys may have multiple versions on disk
// - no suffix: current active key/cert
// - old: previous key/cert from prior to last update
// - new: potential key that's generated, but not yet used - may not have an associated cert yet.
static const char* MQTT_CLIENT_OLD_CERT_FILENAME = "/mqtt_client_cert.pem.old";
static const char* MQTT_CLIENT_CERT_FILENAME = "/mqtt_client_cert.pem";
static const char* MQTT_CLIENT_OLD_KEY_FILENAME = "/mqtt_client_key.pem.old";
static const char* MQTT_CLIENT_NEW_KEY_FILENAME = "/mqtt_client_key.pem.new";
static const char* MQTT_CLIENT_KEY_FILENAME = "/mqtt_client_key.pem";
static const char* MQTT_CLIENT_CSR_FILENAME = "/mqtt_client_csr.pem";

#define PWM_CHANNEL_LEDS        0

#endif
