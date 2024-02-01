#include <Arduino.h>
#include <config.h>
#include <power.h>
#include <WiFi.h>
#include <nvs_config.h>

#include <rom/rtc.h>
#include <driver/rtc_io.h>
#include <driver/adc.h>
#include <esp_wifi.h>
#include <esp_bt.h>

#include <mqtt.h>
#include <i2c.h>
#include <sensors.h>
#include <battery.h>
#include <configManager.h>

// Local logging tag
static const char TAG[] = __FILE__;

extern boolean hasBattery;

namespace Power {

  RTC_NOINIT_ATTR RunMode runmode = RM_UNDEFINED;
  RTC_NOINIT_ATTR struct timeval RTC_sleep_start_time;
  RTC_NOINIT_ATTR TrafficLightStatus trafficLightStatus;
  RTC_NOINIT_ATTR uint32_t RTC_millis = 0;

  void resetRtcVars() {
    runmode = RM_UNDEFINED;
    RTC_millis = 0;
  }

  RunMode getRunMode() {
    return runmode;
  }

  boolean setRunMode(RunMode pMode) {
    if (pMode == RM_UNDEFINED) return false;
    if (pMode == runmode) return true;
    runmode = pMode;
    NVS::writeRunmode(runmode);

    if (model) model->runModeChanged();
    ESP_LOGD(TAG, "Run mode: %s", runmode == RM_UNDEFINED ? "RM_UNDEFINED" : (runmode == RM_LOW ? "conserve power" : "full power"));

    // @TODO: make all necessary calls to enable or disable Wifi, MQTT, task loops, etc, also intervals/modes for sensors
    if (runmode == RM_LOW) {
      // maybe don't do this since we go into sleep soon anyway...
      /*
      // give the sensors loop a chance to clean up
      Sensors::shutDownSensorsLoop();
            mqtt::shutDownMqtt();
            I2C::shutDownI2C();

            if (WiFi.isConnected()) {
              WiFi.disconnect(true, false);
            }
            WiFi.enableSTA(false);
            */
    }
    return true;
  }

  void enableGpioPullDn(gpio_num_t pin) {
    if (gpio_pulldown_en(pin) != ESP_OK) ESP_LOGE(TAG, "error in gpio_pulldown_en: %i", pin);
  }

  void enableRtcHold(gpio_num_t pin) {
    if (rtc_gpio_hold_en(pin) != ESP_OK) ESP_LOGE(TAG, "error in rtc_gpio_hold_en: %i", pin);
  }

  void disableRtcHold(gpio_num_t pin) {
    if (rtc_gpio_hold_dis(pin) != ESP_OK) ESP_LOGE(TAG, "error in rtc_gpio_hold_dis: %i", pin);
    //rtc_gpio_force_hold_dis_all
  }

  void setGpioSleepPullMode(gpio_num_t pin, gpio_pull_mode_t pullMode) {
    if (gpio_sleep_set_pull_mode(pin, pullMode) != ESP_OK) ESP_LOGE(TAG, "error in gpio_sleep_set_pull_mode: %i, %i", pin, pullMode);
  }

  ResetReason afterReset() {
    ResetReason reason = RR_UNDEFINED;

    struct timeval sleep_stop_time;
    uint64_t sleep_time_ms;

    switch (rtc_get_reset_reason(0)) {
      case DEEPSLEEP_RESET:         /**<5, Deep Sleep reset digital core*/
        // keep state
            // calculate time spent in deep sleep
        gettimeofday(&sleep_stop_time, NULL);
        sleep_time_ms =
          (sleep_stop_time.tv_sec - RTC_sleep_start_time.tv_sec) * 1000 +
          (sleep_stop_time.tv_usec - RTC_sleep_start_time.tv_usec) / 1000;
        RTC_millis += sleep_time_ms;
        ESP_LOGI(TAG, "Slept for %u ms, RTC_millis %u, upTime %u", sleep_time_ms, RTC_millis, getUpTime());
        if (model) model->setStatus(trafficLightStatus);
        break;
      case NO_MEAN:
      case POWERON_RESET:           /**<1, Vbat power on reset*/
      case RTCWDT_BROWN_OUT_RESET:  /**<15, Reset when the vdd voltage is not stable*/
      case TG1WDT_SYS_RESET:        /**<8, Timer Group1 Watch dog reset digital core*/
      case RTCWDT_SYS_RESET:        /**<9, RTC Watch dog Reset digital core*/
      case RTCWDT_CPU_RESET:        /**<13, RTC Watch dog Reset CPU*/
      case INTRUSION_RESET:         /**<10, Instrusion tested to reset CPU*/
      case TG0WDT_SYS_RESET:        /**<7, Timer Group0 Watch dog reset digital core*/
      case RTCWDT_RTC_RESET:        /**<16, RTC Watch dog reset digital core and rtc module*/


#if CONFIG_IDF_TARGET_ESP32S3    
      case RTC_SW_CPU_RESET:        /**<12, Software reset CPU*/
      case RTC_SW_SYS_RESET:        /**<3, Software reset digital core*/
      case TG0WDT_CPU_RESET:        /**<11, Time Group0 reset CPU*/
      case TG1WDT_CPU_RESET:        /**<17, Time Group1 reset CPU*/
      case SUPER_WDT_RESET:         /**<18, super watchdog reset digital core and rtc module*/
      case GLITCH_RTC_RESET:        /**<19, glitch reset digital core and rtc module*/
      case EFUSE_RESET:             /**<20, efuse reset digital core*/
      case USB_UART_CHIP_RESET:     /**<21, usb uart reset digital core */
      case USB_JTAG_CHIP_RESET:     /**<22, usb jtag reset digital core */
      case POWER_GLITCH_RESET:      /**<23, power glitch reset digital core and rtc module*/
#elif CONFIG_IDF_TARGET_ESP32 
      case SW_RESET:    /**<3, Software reset digital core*/
      case OWDT_RESET:    /**<4, Legacy watch dog reset digital core*/
      case SDIO_RESET:    /**<6, Reset by SLC module, reset digital core*/
      case TGWDT_CPU_RESET:    /**<11, Time Group reset CPU*/
      case SW_CPU_RESET:    /**<12, Software reset CPU*/
      case EXT_CPU_RESET:    /**<14, for APP CPU, reseted by PRO CPU*/
#endif
      default:
        resetRtcVars();
        reason = FULL_RESET;
        break;
    }

    switch (esp_sleep_get_wakeup_cause()) {
      case ESP_SLEEP_WAKEUP_EXT0:
        reason = WAKE_FROM_BUTTON;
        break;
      case ESP_SLEEP_WAKEUP_TIMER:
        reason = WAKE_FROM_SLEEPTIMER;
        break;
      case ESP_SLEEP_WAKEUP_UNDEFINED:
      case ESP_SLEEP_WAKEUP_ALL:
      case ESP_SLEEP_WAKEUP_EXT1:
      case ESP_SLEEP_WAKEUP_TOUCHPAD:
      case ESP_SLEEP_WAKEUP_ULP:
      case ESP_SLEEP_WAKEUP_GPIO:
      case ESP_SLEEP_WAKEUP_UART:
      case ESP_SLEEP_WAKEUP_WIFI:
      case ESP_SLEEP_WAKEUP_COCPU:
      case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:
      case ESP_SLEEP_WAKEUP_BT:
      default:
        break;
    }

    switch (runmode) {
      case RM_UNDEFINED: ESP_LOGI(TAG, "Run mode: RM_UNDEFINED"); break;
      case RM_LOW: ESP_LOGI(TAG, "Run mode: RM_LOW"); break;
      case RM_FULL: ESP_LOGI(TAG, "Run mode: RM_FULL"); /*setCpuFrequencyMhz(80); */break;
      default: ESP_LOGE(TAG, "Run mode: UNKNOWN (%u)", runmode); break;
    }

    switch (reason) {
      case RR_UNDEFINED: ESP_LOGI(TAG, "Reset reason: RR_UNDEFINED"); break;
      case FULL_RESET: ESP_LOGI(TAG, "Reset reason: FULL_RESET"); break;
      case WAKE_FROM_SLEEPTIMER: ESP_LOGI(TAG, "Reset reason: WAKE_FROM_SLEEPTIMER"); break;
      case WAKE_FROM_BUTTON: ESP_LOGI(TAG, "Reset reason: WAKE_FROM_BUTTON"); break;
      default: ESP_LOGI(TAG, "Reset reason: UNKNOWN (%u)", reason); break;
    }
#ifdef BTN_2
    pinMode(BTN_2, INPUT_PULLUP);
    int btn2 = (digitalRead(BTN_2) ? 0 : 1);
#else
    int btn2 = 1;
#endif

    if (runmode == RM_UNDEFINED) {
      runmode = NVS::readRunmode();
      ESP_LOGE(TAG, "Run mode set to %u from NVS RunMode", runmode);
    }

    if (runmode == RM_UNDEFINED || (reason == WAKE_FROM_BUTTON && btn2 == 1)) {
      runmode = RM_FULL;
      NVS::writeRunmode(runmode);
      ESP_LOGI(TAG, "Run mode: RM_FULL");
    }

    rtc_gpio_deinit((gpio_num_t)BTN_1);
    if (config.neopixelData > 0 && (runmode == RM_FULL || (config.sleepModeOledLed == SLEEP_OLED_ON_LED_ON || config.sleepModeOledLed == SLEEP_OLED_OFF_LED_ON))) {
      pinMode(config.neopixelData, OUTPUT);
      disableRtcHold((gpio_num_t)config.neopixelData);
#ifdef NEO_1_EN
      pinMode(NEO_1_EN, OUTPUT);
      digitalWrite(NEO_1_EN, HIGH);
#endif
      if (runmode == RM_LOW) {
#ifdef NEO_23_EN
        pinMode(NEO_23_EN, OUTPUT);
        digitalWrite(NEO_23_EN, HIGH);
#endif
      } else {
#ifdef NEO_23_EN
        pinMode(NEO_23_EN, OUTPUT);
        digitalWrite(NEO_23_EN, LOW);
#endif
      }
    } else {
#ifdef NEO_1_EN
      pinMode(NEO_1_EN, OUTPUT);
      digitalWrite(NEO_1_EN, LOW);
#endif
#ifdef NEO_23_EN
      pinMode(NEO_23_EN, OUTPUT);
      digitalWrite(NEO_23_EN, LOW);
#endif
    }

    if (config.oledEn > 0) {
      pinMode(config.oledEn, OUTPUT);
      if (runmode == RM_LOW || (config.sleepModeOledLed == SLEEP_OLED_ON_LED_ON || config.sleepModeOledLed == SLEEP_OLED_ON_LED_OFF)) {
        digitalWrite(config.oledEn, HIGH);
      } else {
        digitalWrite(config.oledEn, LOW);
      }
      disableRtcHold((gpio_num_t)config.oledEn);
    }
    if (config.buzzerPin > 0) disableRtcHold((gpio_num_t)config.buzzerPin);
    disableRtcHold((gpio_num_t)LED_PIN);

    enableGpioPullDn(GPIO_NUM_35);    // GPIO SUBSPID
    enableGpioPullDn(GPIO_NUM_36);    // GPIO SUBSPICLK
    enableGpioPullDn(GPIO_NUM_37);    // GPIO SUBSPIQ
    enableGpioPullDn(GPIO_NUM_38);    // GPIO SUBSPIWP
    enableGpioPullDn(GPIO_NUM_39);    // TCK (1.4V ???)
#if CONFIG_IDF_TARGET_ESP32S3    
    enableGpioPullDn(GPIO_NUM_40);    // TDO
    enableGpioPullDn(GPIO_NUM_41);    // TDI
    enableGpioPullDn(GPIO_NUM_42);    // TMS
    enableGpioPullDn(GPIO_NUM_44);    // U0RxD (1.4V ???)
    //    enableGpioPullDn(GPIO_NUM_47);    // GPIO SUBSPICLK_P_DIFF
    //    enableGpioPullDn(GPIO_NUM_48);    // GPIO SUBSPICLK_N_DIFF
#endif

    return reason;
  }

  void prepareForDeepSleep(bool powerDown) {
    ESP_LOGI(TAG, "prepareForDeepSleep(%s)", powerDown ? "true" : "false");

    if (powerDown || config.sleepModeOledLed == SLEEP_OLED_OFF_LED_ON || config.sleepModeOledLed == SLEEP_OLED_OFF_LED_OFF) digitalWrite(config.oledEn, LOW);
    if (powerDown || config.sleepModeOledLed == SLEEP_OLED_ON_LED_OFF || config.sleepModeOledLed == SLEEP_OLED_OFF_LED_OFF) {
#ifdef NEO_1_EN
      digitalWrite(NEO_1_EN, LOW);
      setGpioSleepPullMode((gpio_num_t)NEO_1_EN, GPIO_PULLDOWN_ONLY);
#endif
      digitalWrite(config.neopixelData, HIGH);
      setGpioSleepPullMode((gpio_num_t)config.neopixelData, GPIO_PULLUP_ONLY);
    } else {
#ifdef NEO_1_EN
      digitalWrite(NEO_1_EN, HIGH);
      setGpioSleepPullMode((gpio_num_t)NEO_1_EN, GPIO_PULLUP_ONLY);
#endif
      digitalWrite(config.neopixelData, LOW);
      setGpioSleepPullMode((gpio_num_t)config.neopixelData, GPIO_PULLDOWN_ONLY);
    }
#ifdef NEO_23_EN
    digitalWrite(NEO_23_EN, LOW);
    setGpioSleepPullMode((gpio_num_t)NEO_23_EN, GPIO_PULLDOWN_ONLY);
#endif
    digitalWrite(LED_PIN, LOW);
    digitalWrite(config.buzzerPin, LOW);
    digitalWrite(config.vBatEn, LOW);

    mqtt::shutDownMqtt();
    I2C::shutDownI2C();

    if (WiFi.isConnected()) {
      WiFi.disconnect(true, false);
    }
    WiFi.enableSTA(false);

    adc_power_off();
    esp_wifi_stop();
    esp_bt_controller_disable();

    if (model) trafficLightStatus = model->getStatus();

    enableRtcHold((gpio_num_t)config.neopixelData);
    enableRtcHold((gpio_num_t)config.oledEn);
    setGpioSleepPullMode((gpio_num_t)config.buzzerPin, GPIO_PULLDOWN_ONLY);
    enableRtcHold((gpio_num_t)LED_PIN);
    setGpioSleepPullMode((gpio_num_t)VBAT_EN, GPIO_PULLDOWN_ONLY);

    // don't - it increases power consumption. Use pullup/downn instead.
    //gpio_deep_sleep_hold_en();
  }

  void deepSleep(uint64_t durationInSeconds) {
    prepareForDeepSleep(false);

    int result = esp_sleep_enable_timer_wakeup(durationInSeconds * 1000000UL);
    if (result != ESP_OK) ESP_LOGE(TAG, "error in esp_sleep_enable_timer_wakeup: %i", result);

    // wakeup source
    // EXT0
    result = rtc_gpio_pullup_en((gpio_num_t)BTN_1);
    if (result != ESP_OK) ESP_LOGE(TAG, "error in rtc_gpio_pullup_en: %i", result);

    result = esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_1, 0);
    if (result != ESP_OK) ESP_LOGE(TAG, "error in esp_sleep_enable_ext0_wakeup: %i", result);

    result = rtc_gpio_isolate((gpio_num_t)BTN_1);
    if (result != ESP_OK) ESP_LOGE(TAG, "error in rtc_gpio_isolate: %i", result);

    gettimeofday(&RTC_sleep_start_time, NULL);
    RTC_millis += millis();
    ESP_LOGI(TAG, "Going to sleep now - bye! Time: %u", millis());
    esp_deep_sleep_start();
  }

  void powerDown() {
    prepareForDeepSleep(true);

    // @TODO: turn off all periphals possible!
    // disable Neopixels, turn off all sensors
    // check if RTC hold is required to pin down outputs

    if (esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_RTC_PERIPH");
    if (esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_RTC_SLOW_MEM");
    if (esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_RTC_FAST_MEM");
    if (esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_XTAL");
#if CONFIG_IDF_TARGET_ESP32S3 
    if (esp_sleep_pd_config(ESP_PD_DOMAIN_CPU, ESP_PD_OPTION_OFF) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_CPU");
#endif
    if (esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_OFF) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_RTC8M");
    if (esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF) != ESP_OK) ESP_LOGE(TAG, "Error turning off ESP_PD_DOMAIN_VDDSDIO");
    ESP_LOGI(TAG, "Switching off - goodbye! Time: %u", millis());
    esp_deep_sleep_start();
  }

  uint32_t getUpTime() {
    return RTC_millis + millis();
  }
}