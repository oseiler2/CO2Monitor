#include <globals.h>
#include <Arduino.h>
#include <config.h>

#include <WiFi.h>
#include <sntp.h>
#include <Wire.h>
#include <i2c.h>
#include <esp_event.h>
#include <esp_err.h>

#include <power.h>
#include <configManager.h>
#include <mqtt.h>
#include <sensors.h>
#include <scd30.h>
#include <scd40.h>
#include <sps_30.h>
#include <housekeeping.h>
#include <lcd.h>
#include <trafficLight.h>
#include <neopixel.h>
#include <featherMatrix.h>
#include <buzzer.h>
#include <bme680.h>
#include <wifiManager.h>
#include <ota.h>
#include <model.h>
#include <rtc_osc.h>
#include <sd_card.h>
#include <battery.h>
#include <timekeeper.h>

#if CONFIG_IDF_TARGET_ESP32
#include <hub75.h>
#endif

// Local logging tag
static const char TAG[] = __FILE__;

#if CONFIG_IDF_TARGET_ESP32
#elif CONFIG_IDF_TARGET_ESP32S3
#endif

Model* model;
LCD* lcd;
TrafficLight* trafficLight;
Neopixel* neopixel;
FeatherMatrix* featherMatrix;
#if CONFIG_IDF_TARGET_ESP32
HUB75* hub75;
#endif
Buzzer* buzzer;
SCD30* scd30;
SCD40* scd40;
SPS_30* sps30;
BME680* bme680;

bool hasLEDs = false;
bool hasNeoPixel = false;
bool hasFeatherMatrix = false;
bool hasBuzzer = false;
bool hasHub75 = false;
bool hasSdCard = false;

const uint32_t debounceDelay = 50;
volatile uint32_t lastBtn1DebounceTime = 0;
volatile uint8_t button1State = 0;
uint8_t oldConfirmedButton1State = 0;
volatile uint32_t lastBtn2DebounceTime = 0;
volatile uint8_t button2State = 0;
uint8_t oldConfirmedButton2State = 0;
volatile uint32_t lastBtn3DebounceTime = 0;
volatile uint8_t button3State = 0;
uint8_t oldConfirmedButton3State = 0;
volatile uint32_t lastBtn4DebounceTime = 0;
volatile uint8_t button4State = 0;
uint8_t oldConfirmedButton4State = 0;

void ICACHE_RAM_ATTR button1Handler() {
  button1State = (digitalRead(BTN_1) ? 0 : 1);
  lastBtn1DebounceTime = millis();
}

void ICACHE_RAM_ATTR button2Handler() {
  button2State = (digitalRead(BTN_2) ? 0 : 1);
  lastBtn2DebounceTime = millis();
}

void ICACHE_RAM_ATTR button3Handler() {
  button3State = (digitalRead(BTN_3) ? 0 : 1);
  lastBtn3DebounceTime = millis();
}

void ICACHE_RAM_ATTR button4Handler() {
  button4State = (digitalRead(BTN_4) ? 0 : 1);
  lastBtn4DebounceTime = millis();
}

void stopHub75DMA() {
#if CONFIG_IDF_TARGET_ESP32
  if (hasHub75 && hub75) hub75->stopDMA();
#endif
}

void updateMessage(char const* msg) {
  if (lcd) {
    lcd->updateMessage(msg);
  }
}

void setPriorityMessage(char const* msg) {
  if (lcd) {
    lcd->setPriorityMessage(msg);
  }
}

void clearPriorityMessage() {
  if (lcd) {
    lcd->clearPriorityMessage();
  }
}

void modelUpdatedEvt(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
  if (lcd) lcd->update(mask, oldStatus, newStatus);
  if (hasLEDs && trafficLight) trafficLight->update(mask, oldStatus, newStatus);
  if (hasNeoPixel && neopixel) neopixel->update(mask, oldStatus, newStatus);
  if (hasFeatherMatrix && featherMatrix) featherMatrix->update(mask, oldStatus, newStatus);
#if CONFIG_IDF_TARGET_ESP32
  if (hasHub75 && hub75) hub75->update(mask, oldStatus, newStatus);
#endif
  if (hasBuzzer && buzzer) buzzer->update(mask, oldStatus, newStatus);
  if ((mask & M_PRESSURE) && I2C::scd40Present() && scd40) scd40->setAmbientPressure(model->getPressure());
  if ((mask & M_PRESSURE) && I2C::scd30Present() && scd30) scd30->setAmbientPressure(model->getPressure());
  if ((mask & ~(M_CONFIG_CHANGED | M_VOLTAGE | M_POWER_MODE)) != M_NONE) mqtt::publishSensors(mask);
  if (hasSdCard && ((mask & ~(M_CONFIG_CHANGED | M_VOLTAGE)) != M_NONE)) SdCard::writeEvent(mask, model, newStatus, model->getVoltageInMv());
}

void calibrateCo2SensorCallback(uint16_t co2Reference) {
  if (I2C::scd30Present() && scd30) scd30->calibrateScd30ToReference(co2Reference);
  if (I2C::scd40Present() && scd40) scd40->calibrateScd40ToReference(co2Reference);
}

void setTemperatureOffsetCallback(float temperatureOffset) {
  if (I2C::scd30Present() && scd30) scd30->setTemperatureOffset(temperatureOffset);
  if (I2C::scd40Present() && scd40) scd40->setTemperatureOffset(temperatureOffset);
}

float getTemperatureOffsetCallback() {
  if (I2C::scd30Present() && scd30) return scd30->getTemperatureOffset();
  if (I2C::scd40Present() && scd40) return scd40->getTemperatureOffset();
  return NaN;
}

uint32_t getSPS30AutoCleanInterval() {
  if (I2C::sps30Present && sps30) return sps30->getAutoCleanInterval();
  return 0;
}

boolean setSPS30AutoCleanInterval(uint32_t intervalInSeconds) {
  if (I2C::sps30Present && sps30) return sps30->setAutoCleanInterval(intervalInSeconds);
  return false;
}

boolean cleanSPS30() {
  if (I2C::sps30Present && sps30) return sps30->clean();
  return false;
}

uint8_t getSPS30Status() {
  if (I2C::sps30Present && sps30) return sps30->getStatus();
  return false;
}

void logCoreInfo() {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  ESP_LOGI(TAG,
    "This is ESP32 chip with %d CPU cores, WiFi%s%s, silicon revision "
    "%d, %dMB %s Flash",
    chip_info.cores,
    (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
    (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
    chip_info.revision, spi_flash_get_chip_size() / (1024 * 1024),
    (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded"
    : "external");
  ESP_LOGI(TAG, "Internal Total heap %d, internal Free Heap %d",
    ESP.getHeapSize(), ESP.getFreeHeap());
#ifdef BOARD_HAS_PSRAM
  ESP_LOGI(TAG, "SPIRam Total heap %d, SPIRam Free Heap %d",
    ESP.getPsramSize(), ESP.getFreePsram());
#endif
  ESP_LOGI(TAG, "ChipRevision %d, Cpu Freq %d, SDK Version %s",
    ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
  ESP_LOGI(TAG, "Flash Size %d, Flash Speed %d", ESP.getFlashChipSize(),
    ESP.getFlashChipSpeed());
}

void eventHandler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ESP_LOGD(TAG, "eventHandler IP_EVENT IP_EVENT_STA_GOT_IP");
    Timekeeper::initSntp();
  } else if (event_base == WIFI_EVENT) {
    switch (event_id) {
      case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGD(TAG, "eventHandler WIFI_EVENT WIFI_EVENT_STA_CONNECTED");
        digitalWrite(LED_PIN, HIGH);
        break;
      case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGD(TAG, "eventHandler WIFI_EVENT WIFI_EVENT_STA_DISCONNECTED");
        digitalWrite(LED_PIN, LOW);
        break;
      default:
        ESP_LOGD(TAG, "eventHandler WIFI_EVENT %u", event_id);
        break;
    }
  } else {
    ESP_LOGD(TAG, "eventHandler %s %u", event_base, event_id);
  }
}

Ticker clockTimer;

void showTimeLcd() {
  if (lcd) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    char buf[20];
    strftime(buf, 20, "%d/%m/%Y %H:%M.%S", &timeinfo);
    lcd->updateMessage(buf);
  }
}

void setup() {
  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  ESP_LOGI(TAG, "CO2 Monitor v%s. Built from %s @ %s", APP_VERSION, SRC_REVISION, BUILD_TIMESTAMP);

  RtcOsc::setupRtc();
  model = new Model(modelUpdatedEvt);
  Battery::init(model);

  ResetReason resetReason = Power::afterReset();
  boolean reinitFromSleep = (resetReason == WAKE_FROM_SLEEPTIMER || resetReason == WAKE_FROM_BUTTON);
  if (Power::getPowerMode() == PM_UNDEFINED) {
    Power::setPowerMode(USB);
  }

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  pinMode(BTN_3, INPUT_PULLUP);
  pinMode(BTN_4, INPUT_PULLUP);
  pinMode(OLED_EN, OUTPUT);
  digitalWrite(OLED_EN, HIGH);
  pinMode(NEO_DATA, OUTPUT);
  digitalWrite(NEO_DATA, LOW);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(VBAT_EN, OUTPUT);
  digitalWrite(VBAT_EN, LOW);
  if (!reinitFromSleep) logCoreInfo();

  Timekeeper::init();

  if (Power::getPowerMode() == USB) {
    sntp_servermode_dhcp(1); // needs to be set before Wifi connects and gets DHCP IP

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, eventHandler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, eventHandler, NULL));

    // try to connect with known settings
    WiFi.begin();
  }

  setupConfigManager();
  if (!loadConfiguration(config)) {
    getDefaultConfiguration(config);
    saveConfiguration(config);
  }
  if (!reinitFromSleep) logConfiguration(config);

  hasLEDs = (config.greenLed != 0 && config.yellowLed != 0 && config.redLed != 0);
  hasNeoPixel = (config.neopixelData != 0 && config.neopixelNumber != 0);
  hasFeatherMatrix = (config.featherMatrixClock != 0 && config.featherMatrixData != 0);
#if CONFIG_IDF_TARGET_ESP32
  hasHub75 = (config.hub75B1 != 0 && config.hub75B2 != 0 && config.hub75ChA != 0 && config.hub75ChB != 0 && config.hub75ChC != 0 && config.hub75ChD != 0
    && config.hub75Clk != 0 && config.hub75G1 != 0 && config.hub75G2 != 0 && config.hub75Lat != 0 && config.hub75Oe != 0 && config.hub75R1 != 0 && config.hub75R2 != 0);
#endif
  hasBuzzer = true;
  hasSdCard = SdCard::probe();

  Wire.begin((int)SDA_PIN, (int)SCL_PIN, (uint32_t)I2C_CLK);

  I2C::initI2C(!reinitFromSleep);

  if (I2C::scd30Present()) scd30 = new SCD30(&Wire, model, updateMessage, reinitFromSleep);
  if (I2C::scd40Present()) scd40 = new SCD40(&Wire, model, updateMessage, reinitFromSleep);
  if (I2C::sps30Present()) sps30 = new SPS_30(&Wire, model, updateMessage, reinitFromSleep);
  if (I2C::bme680Present()) bme680 = new BME680(&Wire, model, updateMessage, reinitFromSleep);
  if (I2C::lcdPresent()) lcd = new LCD(&Wire, model, reinitFromSleep);

  if (hasLEDs) trafficLight = new TrafficLight(model, config.redLed, config.yellowLed, config.greenLed, reinitFromSleep);
  if (hasNeoPixel) neopixel = new Neopixel(model, config.neopixelData, config.neopixelNumber, reinitFromSleep);
  if (hasFeatherMatrix) featherMatrix = new FeatherMatrix(model, config.featherMatrixData, config.featherMatrixClock, reinitFromSleep);
#if CONFIG_IDF_TARGET_ESP32
  if (hasHub75) hub75 = new HUB75(model);
#endif
  if (hasBuzzer) buzzer = new Buzzer(model, BUZZER_PIN, reinitFromSleep);
  if (hasSdCard) hasSdCard &= SdCard::setup();
  Sensors::setupSensorsLoop(scd30, scd40, sps30, bme680);

  if (Power::getPowerMode() == USB) {
    mqtt::setupMqtt(
      model,
      calibrateCo2SensorCallback,
      setTemperatureOffsetCallback,
      getTemperatureOffsetCallback,
      getSPS30AutoCleanInterval,
      setSPS30AutoCleanInterval,
      cleanSPS30,
      getSPS30Status);

    xTaskCreatePinnedToCore(mqtt::mqttLoop,  // task function
      "mqttLoop",         // name of task
      8192,               // stack size of task
      (void*)1,           // parameter of the task
      2,                  // priority of the task
      &mqtt::mqttTask,    // task handle
      0);                 // CPU core

    xTaskCreatePinnedToCore(OTA::otaLoop,  // task function
      "otaLoop",          // name of task
      8192,               // stack size of task
      (void*)1,           // parameter of the task
      2,                  // priority of the task
      &OTA::otaTask,      // task handle
      1);                 // CPU core

    if (scd30 || scd40 || sps30 || bme680) {
      Sensors::start(
        "sensorsLoop",      // name of task
        4096,               // stack size of task
        2,                  // priority of the task
        1);                 // CPU core
    }

    housekeeping::cyclicTimer.attach(30, housekeeping::doHousekeeping);

    OTA::setupOta(stopHub75DMA);

    clockTimer.attach(1, showTimeLcd);
  }

  attachInterrupt(BTN_1, button1Handler, CHANGE);
  attachInterrupt(BTN_2, button2Handler, CHANGE);
  attachInterrupt(BTN_3, button3Handler, CHANGE);
  attachInterrupt(BTN_4, button4Handler, CHANGE);

  Battery::readVoltage();

  ESP_LOGI(TAG, "Setup done.");
#ifdef SHOW_DEBUG_MSGS
  if (I2C::lcdPresent()) {
    lcd->updateMessage("Setup done.");
  }
#endif
}

void loop() {
  if (Power::getPowerMode() == BATTERY) {
    Timekeeper::printTime();
    Sensors::runOnce();
    showTimeLcd();
    uint8_t percent = Battery::getBatteryLevelInPercent(model->getVoltageInMv());
    if (percent < 10) {
      ESP_LOGI(TAG, ">>>> Battery critial - turning off !");
      if (hasBuzzer && buzzer) buzzer->alert();
      if (hasNeoPixel && neopixel) neopixel->off();
      if (scd40) scd40->shutdown();
      if (bme680) bme680->shutdown();
      Power::powerDown();
    }
    Power::deepSleep(30);
  }
  /*
    if (buttonState != oldConfirmedButtonState && (millis() - lastBtnDebounceTime) > debounceDelay) {
      oldConfirmedButtonState = buttonState;
      if (oldConfirmedButtonState == 0) {
        digitalWrite(LED_PIN, LOW);
        stopHub75DMA();
        WifiManager::startConfigPortal(updateMessage, setPriorityMessage, clearPriorityMessage);
      }
    }
  */
  if (button1State != oldConfirmedButton1State && (millis() - lastBtn1DebounceTime) > debounceDelay) {
    oldConfirmedButton1State = button1State;
    if (oldConfirmedButton1State == 1) {
      ESP_LOGI(TAG, "Button 1 pressed!");
    }
    if (oldConfirmedButton1State == 0) {
      if (Power::getPowerMode() == USB) {
        digitalWrite(LED_PIN, LOW);
        stopHub75DMA();
        WifiManager::startConfigPortal(updateMessage, setPriorityMessage, clearPriorityMessage);
      }
    }
  }

  if (button2State != oldConfirmedButton2State && (millis() - lastBtn2DebounceTime) > debounceDelay) {
    oldConfirmedButton2State = button2State;
    if (oldConfirmedButton2State == 1) {
      ESP_LOGI(TAG, "Button 2 pressed!");
      ESP_LOGI(TAG, "Uptime %u", Power::getUpTime());
      Timekeeper::printTime();
    }
  }
  if (button3State != oldConfirmedButton3State && (millis() - lastBtn3DebounceTime) > debounceDelay) {
    oldConfirmedButton3State = button3State;
    if (oldConfirmedButton3State == 1) {
      ESP_LOGI(TAG, "Button 3 pressed!");
      ESP_LOGI(TAG, "Uptime %u", Power::getUpTime());
      Power::setPowerMode(BATTERY);
      if (scd40) scd40->setSampleRate(LP_PERIODIC);
      if (bme680) bme680->setSampleRate(ULP);
    }
  }
  if (button4State != oldConfirmedButton4State && (millis() - lastBtn4DebounceTime) > debounceDelay) {
    oldConfirmedButton4State = button4State;
    if (oldConfirmedButton4State == 1) {
      ESP_LOGI(TAG, "Button 4 pressed!");
      Power::deepSleep(10);
    }
  }

  vTaskDelay(pdMS_TO_TICKS(50));
}