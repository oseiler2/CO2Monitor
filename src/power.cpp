#include <Arduino.h>
#include <config.h>
#include <power.h>
#include <WiFi.h>

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

  RTC_NOINIT_ATTR PowerMode powermode = PM_UNDEFINED;
  RTC_NOINIT_ATTR struct timeval RTC_sleep_start_time;
  RTC_NOINIT_ATTR TrafficLightStatus trafficLightStatus;
  RTC_NOINIT_ATTR uint32_t RTC_millis = 0;

  void resetRtcVars() {
    powermode = PM_UNDEFINED;
    RTC_millis = 0;
  }

  PowerMode getPowerMode() {
    return powermode;
  }

  boolean setPowerMode(PowerMode pMode) {
    if (pMode == PM_UNDEFINED) return false;
    if ((!hasBattery || !Battery::batteryPresent()) && pMode == BATTERY) return false;
    if (pMode == powermode) return true;
    powermode = pMode;
    if (model) model->powerModeChanged();
    ESP_LOGI(TAG, "Power mode: %s", powermode == PM_UNDEFINED ? "PM_UNDEFINED" : (powermode == USB ? "USB" : "Battery"));

    // @TODO: make all necessary calls to enable or disable Wifi, MQTT, task loops, etc, also intervals/modes for sensors
    if (powermode == BATTERY) {
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

  ResetReason afterReset() {
    rtc_gpio_deinit((gpio_num_t)BTN_1);
    if (config.neopixelData > 0) disableRtcHold((gpio_num_t)config.neopixelData);
    if (config.oledEn > 0) disableRtcHold((gpio_num_t)config.oledEn);
    if (config.buzzer > 0) disableRtcHold((gpio_num_t)config.buzzer);
    disableRtcHold((gpio_num_t)LED_PIN);
    //  disableRtcHold((gpio_num_t)VBAT_EN); // 46 - bootstrap

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
    enableGpioPullDn(GPIO_NUM_47);    // GPIO SUBSPICLK_P_DIFF
    enableGpioPullDn(GPIO_NUM_48);    // GPIO SUBSPICLK_N_DIFF
#endif

    ResetReason reason = RR_UNDEFINED;

    //    ESP_LOGD(TAG, "=========================== esp_sleep_get_wakeup_cause: %u", esp_sleep_get_wakeup_cause());
    /*
        switch (esp_sleep_get_wakeup_cause()) {
          case ESP_SLEEP_WAKEUP_UNDEFINED: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: ESP_SLEEP_WAKEUP_UNDEFINED"); break;
          case ESP_SLEEP_WAKEUP_ALL: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: ESP_SLEEP_WAKEUP_ALL"); break;
          case ESP_SLEEP_WAKEUP_EXT0: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: ESP_SLEEP_WAKEUP_EXT0"); break;
          case ESP_SLEEP_WAKEUP_EXT1: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: ESP_SLEEP_WAKEUP_EXT1"); break;
          case ESP_SLEEP_WAKEUP_TIMER: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: ESP_SLEEP_WAKEUP_TIMER"); break;
          case ESP_SLEEP_WAKEUP_TOUCHPAD: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: ESP_SLEEP_WAKEUP_TOUCHPAD"); break;
          case ESP_SLEEP_WAKEUP_ULP: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: ESP_SLEEP_WAKEUP_ULP"); break;
          case ESP_SLEEP_WAKEUP_GPIO: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: ESP_SLEEP_WAKEUP_GPIO"); break;
          case ESP_SLEEP_WAKEUP_UART: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: ESP_SLEEP_WAKEUP_UART"); break;
          case ESP_SLEEP_WAKEUP_WIFI: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: ESP_SLEEP_WAKEUP_WIFI"); break;
          case ESP_SLEEP_WAKEUP_COCPU: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: ESP_SLEEP_WAKEUP_COCPU"); break;
          case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG"); break;
          case ESP_SLEEP_WAKEUP_BT: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: ESP_SLEEP_WAKEUP_BT"); break;
          default: ESP_LOGD(TAG, "esp_sleep_get_wakeup_cause: UNKNOWN (%u)", esp_sleep_get_wakeup_cause()); break;
        }
    */

    //    ESP_LOGD(TAG, "Reset reason: %u, wakeup cause: %x, time: %u", rtc_get_reset_reason(0), rtc_get_wakeup_cause(), millis());

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

    switch (powermode) {
      case PM_UNDEFINED: ESP_LOGI(TAG, "Power mode: PM_UNDEFINED"); break;
      case USB: ESP_LOGI(TAG, "Power mode: USB"); break;
      case BATTERY: ESP_LOGI(TAG, "Power mode: BATTERY"); /*setCpuFrequencyMhz(80); */break;
      default: ESP_LOGI(TAG, "Power mode: UNKNOWN (%u)", powermode); break;
    }

    switch (reason) {
      case RR_UNDEFINED: ESP_LOGI(TAG, "Reset reason: RR_UNDEFINED"); break;
      case FULL_RESET: ESP_LOGI(TAG, "Reset reason: FULL_RESET"); break;
      case WAKE_FROM_SLEEPTIMER: ESP_LOGI(TAG, "Reset reason: WAKE_FROM_SLEEPTIMER"); break;
      case WAKE_FROM_BUTTON: ESP_LOGI(TAG, "Reset reason: WAKE_FROM_BUTTON"); break;
      default: ESP_LOGI(TAG, "Reset reason: UNKNOWN (%u)", reason); break;
    }

    return reason;
  }

  void prepareForDeepSleep() {
    //    digitalWrite(config.oledEn, LOW);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(config.neopixelData, LOW);
    digitalWrite(config.buzzer, LOW);
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
    enableRtcHold((gpio_num_t)config.buzzer);
    enableRtcHold((gpio_num_t)LED_PIN);
    //    enableRtcHold((gpio_num_t)VBAT_EN); // 46 - bootstrap
  }

  void deepSleep(uint64_t durationInSeconds) {
    prepareForDeepSleep();

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
    prepareForDeepSleep();
    digitalWrite(config.oledEn, LOW);
    // @TODO: turn odd all periphals possible!
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