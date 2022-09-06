#include <globals.h>
#include <Arduino.h>
#include <config.h>

#include <WiFi.h>
#include <Wire.h>
#include <i2c.h>
#include <esp_event.h>
#include <esp_err.h>

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
#include <hub75.h>
#include <bme680.h>
#include <wifiManager.h>
#include <ota.h>

// Local logging tag
static const char TAG[] = __FILE__;

Model* model;
LCD* lcd;
TrafficLight* trafficLight;
Neopixel* neopixel;
FeatherMatrix* featherMatrix;
HUB75* hub75;
SCD30* scd30;
SCD40* scd40;
SPS_30* sps30;
BME680* bme680;
TaskHandle_t sensorsTask;

bool hasLEDs = false;
bool hasNeoPixel = false;
bool hasFeatherMatrix = false;
bool hasHub75 = false;

const uint32_t debounceDelay = 50;
volatile uint32_t lastBtnDebounceTime = 0;
volatile uint8_t buttonState = 0;
uint8_t oldConfirmedButtonState = 0;
uint32_t lastConfirmedBtnPressedTime = 0;

void ICACHE_RAM_ATTR buttonHandler() {
  buttonState = (digitalRead(TRIGGER_PIN) ? 0 : 1);
  lastBtnDebounceTime = millis();
}

void stopHub75DMA() {
  if (hasHub75 && hub75) hub75->stopDMA();
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
  if (hasHub75 && hub75) hub75->update(mask, oldStatus, newStatus);
  if ((mask & M_PRESSURE) && I2C::scd40Present() && scd40) scd40->setAmbientPressure(model->getPressure());
  if ((mask & M_PRESSURE) && I2C::scd30Present() && scd30) scd30->setAmbientPressure(model->getPressure());
  if ((mask & ~M_CONFIG_CHANGED) != M_NONE) mqtt::publishSensors(mask);
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

void eventHandler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ESP_LOGD(TAG, "eventHandler IP_EVENT IP_EVENT_STA_GOT_IP");
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

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  ESP_LOGI(TAG, "CO2 Monitor v%s. Built from %s @ %s", APP_VERSION, SRC_REVISION, BUILD_TIMESTAMP);

  model = new Model(modelUpdatedEvt);

  logCoreInfo();

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, eventHandler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, eventHandler, NULL));

  // try to connect with known settings
  WiFi.begin();

  setupConfigManager();
  if (!loadConfiguration(config)) {
    getDefaultConfiguration(config);
    saveConfiguration(config);
  }
  logConfiguration(config);

  hasLEDs = (config.greenLed != 0 && config.yellowLed != 0 && config.redLed != 0);
  hasNeoPixel = (config.neopixelData != 0 && config.neopixelNumber != 0);
  hasFeatherMatrix = (config.featherMatrixClock != 0 && config.featherMatrixData != 0);
  hasHub75 = (config.hub75B1 != 0 && config.hub75B2 != 0 && config.hub75ChA != 0 && config.hub75ChB != 0 && config.hub75ChC != 0 && config.hub75ChD != 0
    && config.hub75Clk != 0 && config.hub75G1 != 0 && config.hub75G2 != 0 && config.hub75Lat != 0 && config.hub75Oe != 0 && config.hub75R1 != 0 && config.hub75R2 != 0);

  Wire.begin((int)SDA, (int)SCL, (uint32_t)I2C_CLK);

  I2C::initI2C();

  if (I2C::scd30Present()) scd30 = new SCD30(&Wire, model, updateMessage);
  if (I2C::scd40Present()) scd40 = new SCD40(&Wire, model, updateMessage);
  if (I2C::sps30Present()) sps30 = new SPS_30(&Wire, model, updateMessage);
  if (I2C::bme680Present()) bme680 = new BME680(&Wire, model, updateMessage);
  if (I2C::lcdPresent()) lcd = new LCD(&Wire, model);

  if (hasLEDs) trafficLight = new TrafficLight(model, config.redLed, config.yellowLed, config.greenLed);
  if (hasNeoPixel) neopixel = new Neopixel(model, config.neopixelData, config.neopixelNumber);
  if (hasFeatherMatrix) featherMatrix = new FeatherMatrix(model, config.featherMatrixData, config.featherMatrixClock);
  if (hasHub75) hub75 = new HUB75(model);

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
    "otaLoop",         // name of task
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

  housekeeping::cyclicTimer.attach(30, housekeeping::doHousekeeping);

  OTA::setupOta(stopHub75DMA);

  attachInterrupt(TRIGGER_PIN, buttonHandler, CHANGE);

  ESP_LOGI(TAG, "Setup done.");
#ifdef SHOW_DEBUG_MSGS
  if (I2C::lcdPresent()) {
    lcd->updateMessage("Setup done.");
  }
#endif
}

void loop() {
  if (buttonState != oldConfirmedButtonState && (millis() - lastBtnDebounceTime) > debounceDelay) {
    oldConfirmedButtonState = buttonState;
    if (oldConfirmedButtonState == 1) {
      lastConfirmedBtnPressedTime = millis();
    } else if (oldConfirmedButtonState == 0) {
      uint32_t btnPressTime = millis() - lastConfirmedBtnPressedTime;
      ESP_LOGD(TAG, "lastConfirmedBtnPressedTime - millis() %u", btnPressTime);
      if (btnPressTime < 2000) {
        digitalWrite(LED_PIN, LOW);
        stopHub75DMA();
        WifiManager::startConfigPortal(updateMessage, setPriorityMessage, clearPriorityMessage);
      } else if (btnPressTime > 5000) {
        calibrateCo2SensorCallback(420);
      }
    }
  }

  vTaskDelay(pdMS_TO_TICKS(50));
}