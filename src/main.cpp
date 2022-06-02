#include <globals.h>
#include <Arduino.h>
#include <config.h>
#include <configManager.h>
#include <WiFi.h>

#include <mqtt.h>
#include <scd30.h>
#include <scd40.h>
#include <sps_30.h>
#include <housekeeping.h>
#include <Wire.h>
#include <lcd.h>
#include <trafficLight.h>
#include <neopixel.h>
#include <featherMatrix.h>
#include <hub75.h>
#include <bme680.h>
#include <i2c.h>
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
TaskHandle_t scd30Task;
TaskHandle_t scd40Task;
TaskHandle_t sps30Task;
TaskHandle_t bme680Task;

bool hasLEDs = false;
bool hasNeoPixel = false;
bool hasFeatherMatrix = false;
bool hasHub75 = false;

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
  if (mask != M_NONE) mqtt::publishSensors(mask);
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

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  ESP_LOGI(TAG, "CO2 Monitor (%s) v%s. Built from %s @ %s",
    MODEL, APP_VERSION, GIT_REV, BUILD_TIMESTAMP);

  // try to connect with known settings
  WiFi.begin();

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

  setupConfigManager();
  printFile();
  if (!loadConfiguration(config)) {
    getDefaultConfiguration(config);
    saveConfiguration(config);
    printFile();
  }
  hasLEDs = (config.greenLed != 0 && config.yellowLed != 0 && config.redLed != 0);
  hasNeoPixel = (config.neopixelData != 0 && config.neopixelNumber != 0);
  hasFeatherMatrix = (config.featherMatrixClock != 0 && config.featherMatrixData != 0);
  hasHub75 = (config.hub75B1 != 0 && config.hub75B2 != 0 && config.hub75ChA != 0 && config.hub75ChB != 0 && config.hub75ChC != 0 && config.hub75ChD != 0
    && config.hub75Clk != 0 && config.hub75G1 != 0 && config.hub75G2 != 0 && config.hub75Lat != 0 && config.hub75Oe != 0 && config.hub75R1 != 0 && config.hub75R2 != 0);


  Wire.begin((int)SDA, (int)SCL, (uint32_t)I2C_CLK);

  // allow SPS30 to come up
  I2C::initI2C();

  model = new Model(modelUpdatedEvt);

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
    &mqtt::mqttTask,          // task handle
    0);                 // CPU core

  xTaskCreatePinnedToCore(OTA::otaLoop,  // task function
    "otaLoop",         // name of task
    8192,               // stack size of task
    (void*)1,           // parameter of the task
    2,                  // priority of the task
    &OTA::otaTask,          // task handle
    1);                 // CPU core

  if (I2C::scd30Present()) {
    scd30Task = scd30->start(
      "scd30Loop",        // name of task
      4096,               // stack size of task
      2,                  // priority of the task
      1);                 // CPU core
  }

  if (I2C::scd40Present()) {
    scd40Task = scd40->start(
      "scd40Loop",        // name of task
      4096,               // stack size of task
      2,                  // priority of the task
      1);                 // CPU core
  }

  if (I2C::sps30Present()) {
    sps30Task = sps30->start(
      "sps30Loop",        // name of task
      4096,               // stack size of task
      2,                  // priority of the task
      1);                 // CPU core
  }

  if (I2C::bme680Present()) {
    bme680Task = bme680->start(
      "bme680Loop",       // name of task
      4096,               // stack size of task
      2,                  // priority of the task
      1);                 // CPU core
  }

  housekeeping::cyclicTimer.attach(30, housekeeping::doHousekeeping);

  WifiManager::setupWifi(setPriorityMessage, clearPriorityMessage);

  OTA::setupOta(stopHub75DMA);

  ESP_LOGI(TAG, "Setup done.");
#ifdef SHOW_DEBUG_MSGS
  if (I2C::lcdPresent()) {
    lcd->updateMessage("Setup done.");
}
#endif
}

void loop() {

  if ((digitalRead(TRIGGER_PIN) == LOW)) {
    while (digitalRead(TRIGGER_PIN) == LOW);
    digitalWrite(LED_PIN, LOW);
    stopHub75DMA();
    WifiManager::startConfigPortal(updateMessage, setPriorityMessage, clearPriorityMessage);
  }

  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, LOW);
    WifiManager::setupWifi(setPriorityMessage, clearPriorityMessage);
  } else if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH);
  }
  vTaskDelay(pdMS_TO_TICKS(1000));
}
