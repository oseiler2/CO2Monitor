#ifndef _CONFIG_H
#define _CONFIG_H

#define APP_VERSION           1
#define OTA_APP               "co2monitor"
//#define OTA_POLL

#define TRIGGER_PIN           0
#define LED_PIN               2

#define SCD30_RDY_PIN         35

#define SSD1306_HEIGHT        64

#define HAS_LEDS

#ifdef HAS_LEDS
#define GREEN_LED_PIN         27
#define YELLOW_LED_PIN        26
#define RED_LED_PIN           25
#endif

//#define HAS_NEOPIXEL

#ifdef HAS_NEOPIXEL
#define NEOPIXEL_PIN          16
#define NEOPIXEL_NUM          3
#endif

//#define HAS_FEATHER_MATRIX

#ifdef HAS_FEATHER_MATRIX
#define FEATHER_MATRIX_DATAPIN    27
#define FEATHER_MATRIX_CLOCKPIN   13
#endif

//#define HAS_HUB75

#ifdef HAS_HUB75
#define HUB75_R1 15
#define HUB75_G1 2
#define HUB75_B1 4
#define HUB75_R2 16
#define HUB75_G2 12
#define HUB75_B2 17
#define HUB75_CH_A 5
#define HUB75_CH_B 18
#define HUB75_CH_C 19
#define HUB75_CH_D 14
#define HUB75_CLK 27
#define HUB75_LAT 26
#define HUB75_OE 25
#endif

#define I2C_CLK 50000     // SCD30 recommendation of 50kHz

static const char* CONFIG_FILENAME = "/config.json";

typedef void (*updateMessageCallback_t)(char const*);
typedef void (*setPriorityMessageCallback_t)(char const*);
typedef void (*clearPriorityMessageCallback_t)(void);

#endif
