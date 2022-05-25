# CO2 and Air Quality Monitor

**ESP32 DevKit (30pin) or Feather based**

Inspired by and many thanks to
[make-IoT / CO2-Ampel](https://github.com/make-IoT/CO2-Ampel) and [Umwelt-Campus](https://www.umwelt-campus.de/en/forschung/projekte/iot-werkstatt/translate-to-englisch-ideen-zur-corona-krise)

There are now 2 new PCB designs, one for SMD components and another one solely using easy to solder through-hole components.

## SMD

- ESP 32 Devkit (30 pin)
- SCD40 or SCD30 CO2 sensor
- optional BME680 IAQ/VOC sensor
- optional 128x64 0.96 inch OLED display
- alternatively Feather wing footprint for easy expansion modules
- combined footprints for either 3 Neopixels (WS2812B 5050) or 3 x 5mm red/yellow/green LEDs
- I2C JST-PH connector for additional connectivity (e.g. SPS30 particulate matter sensor)

![](img/SMD-Neopixel.jpg)

## Through hole

[BOM](pcb/CO2%20Monitor%20V0.3%20through-hole/bom.md)

- uses only through hole components and can be hand soldered
- ESP 32 Devkit (30 pin)
- SCD30 NDIR CO2 sensor
- optional 128x64 0.96 inch OLED display
- 3 x 5mm red/yellow/green LEDs
- I2C JST-PH connector for additional connectivity (e.g. SPS30 particulate matter sensor)

![](img/TH-SCD30.jpg)

## First generation

- ESP 32 Devkit (30 pin)
- SCD40 or SCD30 CO2 sensor
- optional BME680 IAQ/VOC sensor
- Feather wing footprint for easy expansion modules
- 3 x 5mm red/yellow/green LEDs
- I2C JST-PH connector for additional connectivity (e.g. SPS30 particulate matter sensor)
- RFM96 LoRa modem and u.Fl/SMA footprint

![](img/SCD30_lcd.jpg)

![](img/SCD40_case.jpg)

## First generation with Neopixel feather wing

![](img/Neopixel-feather.jpg)

# Use case

A large risk of Covid-19 transmission stems from airborne virus particles that can linger in poorly ventilated spaces for hours. This has been recognised by the WHO. To lower the risk of infection, particularly in busier indoor settings, good ventilation and regular air exchange are key.

This is where measuring CO2 levels in rooms can provide a direct and good indication of sufficient ventilation which correlates with reduced viral load and low risk of virus transmission. Good air quality is also important for creating a good learning or work environment.

Poorly ventilated rooms often feel stale and ‘stuffy’, but by the time we can feel that the air quality is already pretty poor and early indications are easily missed. A more sensitive and consistent CO2 monitor can accurately measure CO2 levels and display an easy-to-understand and actionable traffic light indication with orange as an indication of worsening air quality and red as a reminder to open a window.

The data collected by the sensors should be logged and made available for further consumption. It can be visualised a central school wide dashboard and used to establish a baseline and understand patterns or identify rooms that are more difficult to ventilate, as well as potentially alerting or to remotely check a room before another group uses it.

# Firmware

## Wifi

Supports [ESPAsync WiFiManager](https://github.com/khoih-prog/ESPAsync_WiFiManager) to set up wireless credentials and further configuration.

If no wifi credentials have been configured yet it will automatically launch an AP using the SSID `CO2-Monitor-<ESP32mac>`. A password can be configured in `ap_pw.h`.

Once wifi credentials have been configured pressing the `Boot` button on the ESP32 puts the device in configuration mode:

<img src="img/configuration.png">

## MQTT

Sensor readings can be published via MQTT for centralised storage and visualition. Each node is configured with its own id and will then publish under `co2monitor/<id>/up/sensors`. The top level topic `co2monitor` is configurable. Downlink messages to nodes can be sent to each individual node using the id in the topic `co2monitor/<id>/down/<command>`, or to all nodes when omitting the id part `co2monitor/down/<command>`

SCD3x/SCD4x

```
{
  "co2": 752,
  "temperature": "21.6",
  "humidity": "52.1"
}
```

BME680

```
{
  "iaq": 19,
  "temperature": "19.2",
  "humidity": "75.6",
  "pressure": 1014
}
```

SPS30

```
{
  "pm0.5": 16,
  "pm1": 19,
  "pm2.5": 19,
  "pm4": 19,
  "pm10": 19
}
```

Sending `co2monitor/<id>/down/getConfig` will triger the node to reply with its current settings under `co2monitor/<id>/up/config`

```
{
  "appVersion": 1,
  "altitude": 10,
  "yellowThreshold": 700,
  "redThreshold": 900,
  "darkRedThreshold": 1200,
  "ledPwm": 255,
  "mac": "xxyyzz",
  "ip": "1.2.3.4",
  "scd40": true,
  "scd30": true,
  "bme680": true,
  "lcd": true,
  "leds": true,
  "sps30": true,
  "sps30AutoCleanInt": 604800,
  "sps30Status": 0,
  "neopxl": 3,
  "tempOffset": "7.0"
}
```

A message to `co2monitor/<id>/down/setConfig` will set the node's configuration to the provided parameters:

```
{
  "altitude": 10,
  "yellowThreshold": 700,
  "redThreshold": 900,
  "darkRedThreshold": 1200,
  "ledPwm": 255
}
```

A message to `co2monitor/<id>/down/setTemperatureOffset` will set the SCD3x/SCD4x's temperature offset (float, °C):

```
7.0
```

A message to `co2monitor/<id>/down/calibrate` will manually calibrate the SCD3x/SCD4x sensor to the given value:

```
412
```

A message to `co2monitor/<id>/down/setSPS30AutoCleanInterval` will set the SPS30 fan auto-clean interval in seconds to the given value:

```
604800
```

A message to `co2monitor/<id>/down/cleanSPS30` will run a fan clean on the SPS30.

A message to `co2monitor/<id>/down/reboot` will trigger a reset on the node.

## Supported sensors

- [SCD3x CO2, temperature and humidity sensor](https://www.sensirion.com/en/environmental-sensors/carbon-dioxide-sensors/carbon-dioxide-sensors-scd30/)
- [SCD4x CO2, temperature and humidity sensor](https://www.sensirion.com/en/environmental-sensors/carbon-dioxide-sensors/carbon-dioxide-sensor-scd4x/)
- [BME680 IAQ, VOC, temperature and humidity sensor](https://www.bosch-sensortec.com/products/environmental-sensors/gas-sensors/bme680/)
- [SPS30 Small Particulate matter sensor](https://sensirion.com/products/catalog/SPS30/)

## Supported displays

- [128x32 OLED feather wing](https://www.adafruit.com/product/2900)
- [SSD1306 based displays](https://www.aliexpress.com/wholesale?SearchText=128X64+SSD1306)
- [Dot Star feather wing](https://www.adafruit.com/product/3449)
- [Neopixel feather wing](https://www.adafruit.com/product/2945)
- Neopixel strips
- [HUB75 based RBG Matrix panels](https://www.adafruit.com/?q=RGB+LED+Matrix+Panel&sort=BestMatch)

# Sensors

The presence of supported I2C based sensors/displays will be automatically detected on start-up.

## SCD3x

A SCD3x NDIR sensor can be connected to the designated footprint and will provide CO2, temperature and humidity readings via I2C. It supports a separate ready signal which is connected to the ESP32.

## SCD4x

A SCD4x sensor can be soldered directly onto the pcb and provides CO2, temperature and humidity readings via I2C.

## BME680

A BME680 sensor can be directly soldered onto the dedicates footprint or connected via the I2C JST header [Adafruit BME680](https://www.adafruit.com/product/3660), [STEMMA to JST SH Cable](https://www.adafruit.com/product/4424) to provide additional VOC and AIQ measurements.

## SPS30 Particulate matter sensor

The SPS30 sensor can be connected to the I2C and 5V JST connectors of the pcb.

| fn  | I2C | 5V  | SPS30 |
| --- | --- | --- | ----- |
| VDD |     | +5V | 1     |
| SDA | SDA |     | 2     |
| SCL | SCL |     | 3     |
| SEL | GND |     | 4     |
| GND |     | GND | 5     |

## other

Other I2C based sensors can be wired using the JST-PH I2C header.

# Hardware

The board was designed to use an ESP32 or a Feather controller

## ESP32

When an ESP32 is used the feather footprint can be used to drive feather wings. Since some pins are shared with other components please check carefully before connecting a feather.

## Feather

A feather controller can be used to drive the sensors and LEDs, but unfortunately not the RFM96.

## Pin mapping

| ESP32 Devkit (30 pin) | fn       | Feather pin | fn    | LEDs   | SCD3x/4x/BME680/SPS30 | Neopixel |
| --------------------- | -------- | ----------- | ----- | ------ | --------------------- | -------- |
| 1                     | EN       |             |       |        |                       |          |
| 2                     | VP-D36   | 9           | A4    |        |                       |          |
| 3                     | VN-D39   | 8           | A3    |        |                       |          |
| 4                     | D34      | 7           | A2    |        |                       |          |
| 5                     | D35      |             |       |        | RDY (SCD3x)           |          |
| 6                     | D32      | 20          | D6    |        |                       |          |
| 7                     | D33      | 22          | D10   |        |                       |          |
| 8                     | D25      | 6           | A1    | Red    |                       |          |
| 9                     | D26      | 5           | A0    | Yellow |                       |          |
| 10                    | D27      | 23          | D11   | Green  |                       |          |
| 11                    | D14      | 19          | D5    |        |                       |          |
| 12                    | D12      | 24          | D12   |        |                       |          |
| 13                    | D13      | 25          | D13   |        |                       |          |
| 14                    | GND      |             |       |        |                       |          |
| 15                    | V_in     | 26          | V_USB |        |                       |          |
| 16                    | 3V3      | 2           | 3V3   |        |                       |          |
| 17                    | GND      | 4           | GND   |        |                       |          |
| 18                    | D15      | 21          | D9    |        |                       |          |
| 19                    | D2       |             |       |        |                       |          |
| 20                    | D4       | 10          | A5    |        |                       |          |
| 21                    | D16      |             |       |        |                       | DIN      |
| 22                    | D17      |             |       |        |                       |          |
| 23                    | D5/CS0   |             |       |        |                       |          |
| 24                    | D18/SCK  | 11          | SCK   |        |                       |          |
| 25                    | D19/MISO | 13          | MISO  |        |                       |          |
| 26                    | D21/SDA  | 17          | SDA   |        | SDA                   |          |
| 27                    | D3/Rx    | 14          | Rx    |        |                       |          |
| 28                    | D1/TxD   | 15          | Tx    |        |                       |          |
| 29                    | D22/SCL  | 18          | SCL   |        | SCL                   |          |
| 30                    | D23/MOSI | 12          | MOSI  |        |                       |          |
|                       |          | 1           | RST   |        |                       |
|                       |          | 3           | AREF  |        |                       |
|                       |          | 16          | aux   |        |                       |
|                       |          | 27          | En    |        |                       |
|                       |          | 28          | VBat  |        |                       |

## LEDs

The board has room for 3 x 5mm LEDs for traffic light style indication. Each is driven by a transistor to relieve the ESP32/Feather GPIO pins. High intensity LEDs allow for good visibility, but can also be dimmed using the ESP32 internal PWM channels if needed.

- [Green, 12.5cd, 30°, 20 mA, 3.2 V](https://nz.element14.com/cree/lc503fpg1-30q-a3-00001/led-5mm-green/dp/1648988) (use 5Ω resistor)
- [Yellow, 9.3cd, 30°, 20 mA, 2.1 V](https://nz.element14.com/broadcom-limited/hlmp-el3g-vx0dd/led-amber-9-3cd-590nm-th/dp/2900814) (use 60Ω resistor)
- [Red, 9.3cd, 30°, 20 mA, 2.1 V](https://nz.element14.com/broadcom-limited/hlmp-eg3a-wx0dd/led-5mm-alingap-red-30deg/dp/1706677) (use 60Ω resistor)

In `config.h` uncomment `HAS_LEDS` and check the `GREEN_LED_PIN`, `YELLOW_LED_PIN` and `NEOPRED_LED_PINIXEL_NUM` settings

```
#define HAS_LEDS
#define GREEN_LED_PIN 27
#define YELLOW_LED_PIN 26
#define RED_LED_PIN 25
```

## Neopixel

In `config.h` uncomment `HAS_NEOPIXEL` and check the `NEOPIXEL_PIN` and `NEOPIXEL_NUM` settings

```
#define HAS_NEOPIXEL
#define NEOPIXEL_PIN          3   // pin
#define NEOPIXEL_NUM          16  // number ofNeopixels
```

## Feather wings

### [6 x 12 DotStar](https://www.adafruit.com/product/3449)

In `config.h` uncomment `#define HAS_FEATHER_MATRIX` and check the `FEATHER_MATRIX_DATAPIN` and `FEATHER_MATRIX_CLOCKPIN` settings

```
#define HAS_FEATHER_MATRIX
#define FEATHER_MATRIX_DATAPIN    27
#define FEATHER_MATRIX_CLOCKPIN   13
```

### [128x32 OLED](https://www.adafruit.com/product/2900)

In `config.h` change

```
#define SSD1306_HEIGHT  32
```

### [128x64 OLED](https://www.aliexpress.com/wholesale?SearchText=128X64+SSD1306)

In `config.h` set

```
#define SSD1306_HEIGHT  64
```

### [64x32 RBG Matrix panel](https://www.adafruit.com/?q=64x32+RGB+LED+Matrix&sort=BestMatch)

in `config.h` set pin mappings as needed, e.g.

```
#define HUB75_R1 19
#define HUB75_G1 18
#define HUB75_B1 5
#define HUB75_R2 17
#define HUB75_G2 16
#define HUB75_B2 4
#define HUB75_CH_A 13
#define HUB75_CH_B 12
#define HUB75_CH_C 14
#define HUB75_CH_D 27
#define HUB75_CLK 26
#define HUB75_LAT 25
#define HUB75_OE 33
```

## RFM96 (first generation)

This allows adding a LoRa modem when no WiFi is available to transmit measurements.
