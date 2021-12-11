#ifndef _CONFIG_H
#define _CONFIG_H

#define TRIGGER_PIN           0
#define LED_PIN               2

#define SCD30_RDY_PIN         35
//#define SCD30_RDY_PIN         19

#define SSD1306_HEIGHT        32

#define GREEN_LED_PIN         27
#define YELLOW_LED_PIN        26
#define RED_LED_PIN           25

//#define NEOPIXEL_PIN          4
//#define NEOPIXEL_NUM          8

#define I2C_CLK 50000     // SCD30 recommendation of 50kHz

static const char* CONFIG_FILENAME = "/config.json";

typedef void (*updateMessageCallback_t)(char const*);

#endif
