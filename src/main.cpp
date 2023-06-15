#include <logging.h>
#include <globals.h>
#include <Arduino.h>
#include <config.h>

#include <WiFi.h>
#include <sntp.h>
#include <Wire.h>
#include <i2c.h>
#include <esp_event.h>
#include <esp_err.h>
#include <esp_task_wdt.h>

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
#include <neopixelMatrix.h>
#include <featherMatrix.h>
#include <hub75.h>
#include <buzzer.h>
#include <bme680.h>
#include <wifiManager.h>
#include <ota.h>
#include <model.h>
#include <rtc_osc.h>
#include <sd_card.h>
#include <battery.h>
#include <timekeeper.h>
#include <menu.h>

// Local logging tag
static const char TAG[] = __FILE__;

Model* model;
LCD* lcd;
TrafficLight* trafficLight;
Neopixel* neopixel;
NeopixelMatrix* neopixelMatrix;
FeatherMatrix* featherMatrix;
HUB75* hub75;
Buzzer* buzzer;
SCD30* scd30;
SCD40* scd40;
SPS_30* sps30;
BME680* bme680;
TaskHandle_t sensorsTask;
TaskHandle_t wifiManagerTask;
TaskHandle_t neopixelMatrixTask;

bool hasLEDs = false;
bool hasNeoPixel = false;
bool hasFeatherMatrix = false;
bool hasBuzzer = false;
bool hasNeopixelMatrix = false;
bool hasHub75 = false;
bool hasSdSlot = false;
bool hasSdCard = false;
bool hasBtn2 = false;
bool hasBtn3 = false;
bool hasBtn4 = false;
bool hasOledEnable = false;
bool hasBattery = false;

volatile uint8_t wifiDisconnected = 0;
uint32_t lastWifiReconnectAttempt = 0;

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
  button2State = (digitalRead(config.btn2) ? 0 : 1);
  lastBtn2DebounceTime = millis();
}

void ICACHE_RAM_ATTR button3Handler() {
  button3State = (digitalRead(config.btn3) ? 0 : 1);
  lastBtn3DebounceTime = millis();
}

void ICACHE_RAM_ATTR button4Handler() {
  button4State = (digitalRead(config.btn4) ? 0 : 1);
  lastBtn4DebounceTime = millis();
}

void prepareOta() {
  if (hasHub75 && hub75) hub75->stopDMA();
  if (hasNeopixelMatrix && neopixelMatrix) {
    hasNeopixelMatrix = false;
    neopixelMatrix->stop();
  }
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
  if (hasNeopixelMatrix && neopixelMatrix) neopixelMatrix->update(mask, oldStatus, newStatus);
  if (hasHub75 && hub75) hub75->update(mask, oldStatus, newStatus);
  if (hasBuzzer && buzzer) buzzer->update(mask, oldStatus, newStatus);
  if ((mask & M_PRESSURE) && I2C::scd40Present() && scd40) scd40->setAmbientPressure(model->getPressure());
  if ((mask & M_PRESSURE) && I2C::scd30Present() && scd30) scd30->setAmbientPressure(model->getPressure());
  if ((mask & ~(M_CONFIG_CHANGED | M_VOLTAGE | M_POWER_MODE)) != M_NONE) {
    char buf[8];
    DynamicJsonDocument* doc = new DynamicJsonDocument(512);
    if (mask & M_CO2) (*doc)["co2"] = model->getCo2();
    if (mask & M_TEMPERATURE) {
      sprintf(buf, "%.1f", model->getTemperature());
      (*doc)["temperature"] = buf;
    }
    if (mask & M_HUMIDITY) {
      sprintf(buf, "%.1f", model->getHumidity());
      (*doc)["humidity"] = buf;
    }
    if (mask & M_PRESSURE) (*doc)["pressure"] = model->getPressure();
    if (mask & M_IAQ) (*doc)["iaq"] = model->getIAQ();
    if (mask & M_PM0_5) (*doc)["pm0.5"] = model->getPM0_5();
    if (mask & M_PM1_0) (*doc)["pm1"] = model->getPM1();
    if (mask & M_PM2_5) (*doc)["pm2.5"] = model->getPM2_5();
    if (mask & M_PM4) (*doc)["pm4"] = model->getPM4();
    if (mask & M_PM10) (*doc)["pm10"] = model->getPM10();
    mqtt::publishSensors(doc);
  }
  if (hasSdCard && ((mask & ~(M_CONFIG_CHANGED | M_VOLTAGE)) != M_NONE)) SdCard::writeEvent(mask, model, newStatus, model->getVoltageInMv());
}

void configChanged() {
  model->configurationChanged();
}

void calibrateCo2SensorCallback(uint16_t co2Reference) {
  ESP_LOGI(TAG, "Starting calibration");
  if (lcd) lcd->setPriorityMessage("Starting calibration");
  if (I2C::scd30Present() && scd30) scd30->calibrateScd30ToReference(co2Reference);
  if (I2C::scd40Present() && scd40) scd40->calibrateScd40ToReference(co2Reference);
  vTaskDelay(pdMS_TO_TICKS(200));
  if (lcd) lcd->clearPriorityMessage();
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
  esp_task_wdt_init(20, true);
  Serial.begin(115200);
  esp_log_set_vprintf(logging::logger);
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  ESP_LOGI(TAG, "CO2 Monitor v%s. Built from %s @ %s", APP_VERSION, SRC_REVISION, BUILD_TIMESTAMP);

  RtcOsc::setupRtc();
  model = new Model(modelUpdatedEvt);
  Battery::init(model);

  setupConfigManager();
  if (!loadConfiguration(config)) {
    getDefaultConfiguration(config);
    saveConfiguration(config);
  }

  ResetReason resetReason = Power::afterReset();
  boolean reinitFromSleep = (resetReason == WAKE_FROM_SLEEPTIMER || resetReason == WAKE_FROM_BUTTON);
  if (Power::getPowerMode() == PM_UNDEFINED) {
    Power::setPowerMode(USB);
  }

  if (!reinitFromSleep) logConfiguration(config);

  if (!reinitFromSleep) logCoreInfo();

  Timekeeper::init();

  if (Power::getPowerMode() == USB) {
    sntp_servermode_dhcp(1); // needs to be set before Wifi connects and gets DHCP IP

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, WifiManager::eventHandler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, WifiManager::eventHandler, NULL, NULL));

    WifiManager::setupWifiManager("CO2-Monitor", getConfigParameters(), false, true,
      updateMessage, setPriorityMessage, clearPriorityMessage, configChanged);

  }

  hasLEDs = (config.greenLed != 0 && config.yellowLed != 0 && config.redLed != 0);
  hasNeoPixel = (config.neopixelData != 0 && config.neopixelNumber != 0
    && (Power::getPowerMode() == USB || (config.sleepModeOledLed == SLEEP_OLED_ON_LED_ON || config.sleepModeOledLed == SLEEP_OLED_OFF_LED_ON)));
  hasFeatherMatrix = (config.featherMatrixClock != 0 && config.featherMatrixData != 0);
  hasNeopixelMatrix = (config.neopixelMatrixData != 0 && config.matrixColumns != 0 && config.matrixRows != 0);
  hasHub75 = (config.hub75B1 != 0 && config.hub75B2 != 0 && config.hub75ChA != 0 && config.hub75ChB != 0 && config.hub75ChC != 0 && config.hub75ChD != 0
    && config.hub75Clk != 0 && config.hub75G1 != 0 && config.hub75G2 != 0 && config.hub75Lat != 0 && config.hub75Oe != 0 && config.hub75R1 != 0 && config.hub75R2 != 0);
  hasBuzzer = config.buzzerPin != 0;
  hasSdSlot = (config.sdDetect != 0 && config.sdDat0 != 0 && config.sdDat1 != 0 && config.sdDat2 != 0 && config.sdDat3 != 0 && config.sdClk != 0 && config.sdCmd != 0);
  hasBtn2 = config.btn2 != 0;
  hasBtn3 = config.btn3 != 0;
  hasBtn4 = config.btn4 != 0;
  hasOledEnable = config.oledEn != 0;
  hasBattery = config.vBatAdc != 0 && config.vBatEn > 0;

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  pinMode(BTN_1, INPUT_PULLUP);
  if (hasBtn2) pinMode(config.btn2, INPUT_PULLUP);
  if (hasBtn3) pinMode(config.btn3, INPUT_PULLUP);
  if (hasBtn4) pinMode(config.btn4, INPUT_PULLUP);
  if (hasOledEnable
    && (Power::getPowerMode() == USB || (config.sleepModeOledLed == SLEEP_OLED_ON_LED_ON || config.sleepModeOledLed == SLEEP_OLED_ON_LED_OFF))) {
    pinMode(config.oledEn, OUTPUT);
    digitalWrite(config.oledEn, HIGH);
  }
  if (hasNeoPixel) {
    pinMode(config.neopixelData, OUTPUT);
    digitalWrite(config.neopixelData, LOW);
  }
  if (hasBuzzer) {
    pinMode(config.buzzerPin, OUTPUT);
    digitalWrite(config.buzzerPin, LOW);
  }
  if (hasBattery) {
    pinMode(config.vBatEn, OUTPUT);
    digitalWrite(config.vBatEn, LOW);
  }
  if (hasSdSlot) pinMode(config.sdDetect, INPUT);
  hasSdCard = hasSdSlot && SdCard::probe();

  Wire.begin((int)SDA_PIN, (int)SCL_PIN, (uint32_t)I2C_CLK);
  I2C::initI2C(!reinitFromSleep);

  if (I2C::scd30Present()) scd30 = new SCD30(&Wire, model, updateMessage, reinitFromSleep);
  if (I2C::scd40Present()) scd40 = new SCD40(&Wire, model, updateMessage, reinitFromSleep);
  if (I2C::sps30Present()) sps30 = new SPS_30(&Wire, model, updateMessage, reinitFromSleep);
  if (I2C::bme680Present()) bme680 = new BME680(&Wire, model, updateMessage, reinitFromSleep);
  if (I2C::lcdPresent()
    && (Power::getPowerMode() == USB || (config.sleepModeOledLed == SLEEP_OLED_ON_LED_ON || config.sleepModeOledLed == SLEEP_OLED_ON_LED_OFF))
    ) lcd = new LCD(&Wire, model, reinitFromSleep);

  if (hasLEDs) trafficLight = new TrafficLight(model, config.redLed, config.yellowLed, config.greenLed, reinitFromSleep);
  if (hasNeoPixel) neopixel = new Neopixel(model, config.neopixelData, config.neopixelNumber, reinitFromSleep);
  if (hasFeatherMatrix) featherMatrix = new FeatherMatrix(model, config.featherMatrixData, config.featherMatrixClock, reinitFromSleep);
  if (hasNeopixelMatrix) neopixelMatrix = new NeopixelMatrix(model, config.neopixelMatrixData, config.matrixColumns, config.matrixRows, config.matrixLayout);
  if (hasHub75) hub75 = new HUB75(model);
  if (hasBuzzer) buzzer = new Buzzer(model, config.buzzerPin, reinitFromSleep);
  if (hasSdCard)  hasSdCard &= SdCard::setup();

  Sensors::setupSensorsLoop(scd30, scd40, sps30, bme680);

  if (Power::getPowerMode() == USB) {
    mqtt::setupMqtt(
      calibrateCo2SensorCallback,
      setTemperatureOffsetCallback,
      getTemperatureOffsetCallback,
      getSPS30AutoCleanInterval,
      setSPS30AutoCleanInterval,
      cleanSPS30,
      getSPS30Status,
      configChanged);

    char msg[128];
    sprintf(msg, "Reset reason: %u", resetReason);
    mqtt::publishStatusMsg(msg);

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

    Sensors::setupSensorsLoop(scd30, scd40, sps30, bme680);
    sensorsTask = Sensors::start(
      "sensorsLoop",      // name of task
      4096,               // stack size of task
      2,                  // priority of the task
      1);                 // CPU core

    if (hasNeopixelMatrix) {
      neopixelMatrixTask = neopixelMatrix->start(
        "neopixelMatrixLoop",
        4096,
        3,
        1);
    }

    wifiManagerTask = WifiManager::start(
      "wifiManagerLoop",  // name of task
      8192,               // stack size of task
      2,                  // priority of the task
      1);                 // CPU core


    housekeeping::cyclicTimer.attach(30, housekeeping::doHousekeeping);

    OTA::setupOta(prepareOta, setPriorityMessage, clearPriorityMessage);

    clockTimer.attach(1, showTimeLcd);
  }
  attachInterrupt(BTN_1, button1Handler, CHANGE);
  if (hasBtn2) attachInterrupt(config.btn2, button2Handler, CHANGE);
  if (hasBtn3) attachInterrupt(config.btn3, button3Handler, CHANGE);
  if (hasBtn4) attachInterrupt(config.btn4, button4Handler, CHANGE);

  if (hasBattery) Battery::readVoltage();

  ESP_LOGI(TAG, "Setup done.");
#ifdef SHOW_DEBUG_MSGS
  if (lcd) {
    lcd->updateMessage("Setup done.");
  }
#endif
}

void loop() {
  if (Power::getPowerMode() == BATTERY) {
    Timekeeper::printTime();
    Sensors::runOnce();
    showTimeLcd();
    if (hasBattery) {
      uint8_t percent = Battery::getBatteryLevelInPercent(model->getVoltageInMv());
      if (percent < 15) {
        ESP_LOGI(TAG, ">>>> Battery critial - turning off !");
        if (hasBuzzer && buzzer) buzzer->alert();
        if (hasNeoPixel && neopixel) neopixel->off();
        if (scd40) scd40->shutdown();
        if (bme680) bme680->shutdown();
        Power::powerDown();

      }
    }
    if ((config.sleepModeOledLed == SLEEP_OLED_ON_LED_OFF || config.sleepModeOledLed == SLEEP_OLED_OFF_LED_OFF) && hasNeoPixel && neopixel) neopixel->off();
    Power::deepSleep(30);
  }
  if (button1State != oldConfirmedButton1State && (millis() - lastBtn1DebounceTime) > debounceDelay) {
    oldConfirmedButton1State = button1State;
    if (oldConfirmedButton1State == 1) {
      ESP_LOGI(TAG, "Button 1 pressed!");
      Menu::button1Pressed();
    }
  }

  if (hasBtn2 && button2State != oldConfirmedButton2State && (millis() - lastBtn2DebounceTime) > debounceDelay) {
    oldConfirmedButton2State = button2State;
    if (oldConfirmedButton2State == 1) {
      ESP_LOGI(TAG, "Button 2 pressed!");
      Menu::button2Pressed();
    }
  }
  if (hasBtn3 && button3State != oldConfirmedButton3State && (millis() - lastBtn3DebounceTime) > debounceDelay) {
    oldConfirmedButton3State = button3State;
    if (oldConfirmedButton3State == 1) {
      ESP_LOGI(TAG, "Button 3 pressed!");
      Menu::button3Pressed();
    }
  }
  if (hasBtn4 && button4State != oldConfirmedButton4State && (millis() - lastBtn4DebounceTime) > debounceDelay) {
    oldConfirmedButton4State = button4State;
    if (oldConfirmedButton4State == 1) {
      ESP_LOGI(TAG, "Button 4 pressed!");
      Menu::button4Pressed();
    }
  }

  vTaskDelay(pdMS_TO_TICKS(5));
}