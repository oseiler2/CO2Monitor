#include "globals.h"
#include <config.h>
#include <scd30.h>
#include <Arduino.h>

#include <model.h>
#include <configManager.h>
#include <i2c.h>
#include <esp32-hal-timer.h>
#include "freertos/FreeRTOS.h"

// Local logging tag
static const char TAG[] = __FILE__;

#define MAX_RETRY 5

const uint32_t X_CMD_DATA_READY = bit(1);

TaskHandle_t handleForInt;

static void IRAM_ATTR measurementReady() {
  BaseType_t high_task_awoken = pdFALSE;
  if (handleForInt)
    xTaskNotifyFromISR(handleForInt, X_CMD_DATA_READY, eSetBits, &high_task_awoken);
}

SCD30::SCD30(TwoWire* wire, Model* _model, updateMessageCallback_t _updateMessageCallback) {
  this->model = _model;
  this->updateMessageCallback = _updateMessageCallback;
  this->scd30 = new Adafruit_SCD30();

  if (!I2C::takeMutex(pdMS_TO_TICKS(portMAX_DELAY))) return;

  uint8_t retry = 0;
  while (retry < MAX_RETRY && !scd30->begin(SCD30_I2CADDR_DEFAULT, wire, 0)) retry++;
  if (retry >= MAX_RETRY) {
    ESP_LOGW(TAG, "Failed to find SCD30 chip");
  }
  Wire.setClock(I2C_CLK);
  ESP_LOGI(TAG, "Wire.getClock(): %u", Wire.getClock());

  retry = 0;
  while (retry < MAX_RETRY && !scd30->setMeasurementInterval(15)) retry++;
  if (retry >= MAX_RETRY) {
    ESP_LOGW(TAG, "Failed to set measurement interval");
  }
  ESP_LOGD(TAG, "Measurement interval: %u sec", scd30->getMeasurementInterval());

  ESP_LOGD(TAG, "Ambient pressure offset: %u mBar", scd30->getAmbientPressureOffset());

  retry = 0;
  while (retry < MAX_RETRY && !scd30->setAltitudeOffset(config.altitude)) retry++;
  if (retry >= MAX_RETRY) {
    ESP_LOGW(TAG, "Failed to set altitude offset");
  }
  ESP_LOGD(TAG, "Altitude offset: %u m", scd30->getAltitudeOffset());

  /*
    retry = 0;
    while (retry < MAX_RETRY && !scd30->setTemperatureOffset(430)) retry++;
    if (retry >= MAX_RETRY) {
      ESP_LOGW(TAG, "Failed to set temperature offset");
    }
  */

  ESP_LOGD(TAG, "Temperature offset: %.1f C", (float)scd30->getTemperatureOffset() / 100.0);

  ESP_LOGD(TAG, "Forced Recalibration reference: %u ppm", scd30->getForcedCalibrationReference());

  retry = 0;
  while (retry < MAX_RETRY && !scd30->selfCalibrationEnabled(true)) retry++;
  if (retry >= MAX_RETRY) {
    ESP_LOGW(TAG, "Failed to enable self calibration");
  }
  ESP_LOGD(TAG, "Self calibration %s", scd30->selfCalibrationEnabled() ? "enabled" : "disabled");

  retry = 0;
  while (retry < MAX_RETRY && !scd30->startContinuousMeasurement()) retry++;
  if (retry >= MAX_RETRY) {
    ESP_LOGW(TAG, "Failed to start continuous measurement");
  }

  pinMode(SCD30_RDY_PIN, INPUT);

  I2C::giveMutex();
  ESP_LOGD(TAG, "SCD30 initialised");
}

SCD30::~SCD30() {
  detachInterrupt(SCD30_RDY_PIN);
  if (this->task) vTaskDelete(this->task);
  if (this->scd30) delete scd30;
}

boolean SCD30::readScd30() {
  this->updateMessageCallback("readScd30");
  if (!I2C::takeMutex(pdMS_TO_TICKS(1000))) return false;
  boolean read = scd30->dataReady() && scd30->read();
  I2C::giveMutex();
  if (read) {
    ESP_LOGD(TAG, "Temp: %.1fC, rH: %.1f%%, CO2:  %.0fppm", scd30->temperature, scd30->relative_humidity, scd30->CO2);
    updateMessageCallback("");
    model->updateModel(scd30->CO2, scd30->temperature, scd30->relative_humidity);
    return true;
  } else {
    updateMessageCallback("sensor read error");
    ESP_LOGW(TAG, "Error reading sensor data");
  }
  return false;
}

boolean SCD30::calibrateScd30ToReference(uint16_t co2Reference) {
  if (!I2C::takeMutex(pdMS_TO_TICKS(1000))) return false;
  uint8_t retry = 0;
  while (retry++ < MAX_RETRY && !scd30->forceRecalibrationWithReference(co2Reference));
  ESP_LOGD(TAG, "co2Reference: %u, result %s", co2Reference, (retry < MAX_RETRY) ? "true" : "false");
  I2C::giveMutex();
  return (retry < MAX_RETRY);
}

float SCD30::getTemperatureOffset() {
  if (!I2C::takeMutex(pdMS_TO_TICKS(1000))) return false;
  float temperatureOffset = scd30->getTemperatureOffset() / 100.0;
  ESP_LOGD(TAG, "Temperature offset: %.1f C", temperatureOffset);
  I2C::giveMutex();
  return temperatureOffset;
}

boolean SCD30::setTemperatureOffset(float temperatureOffset) {
  if (!I2C::takeMutex(pdMS_TO_TICKS(1000))) return false;
  uint8_t retry = 0;
  while (retry < MAX_RETRY && !scd30->setTemperatureOffset(floor(temperatureOffset * 100))) retry++;
  if (retry >= MAX_RETRY)
    ESP_LOGW(TAG, "Failed to set temperature offset");
  I2C::giveMutex();
  return (retry < MAX_RETRY);
}

TaskHandle_t SCD30::start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core) {
  xTaskCreatePinnedToCore(
    this->scd30Loop,  // task function
    name,             // name of task
    stackSize,        // stack size of task
    this,             // parameter of the task
    priority,         // priority of the task
    &task,            // task handle
    core);            // CPU core
  handleForInt = this->task;
  attachInterrupt(SCD30_RDY_PIN, measurementReady, RISING);
  return this->task;
}

void SCD30::scd30Loop(void* pvParameters) {
  SCD30* instance = (SCD30*)pvParameters;
  uint32_t taskNotification;
  BaseType_t notified;

  while (1) {
    notified = xTaskNotifyWait(0x00,  // Don't clear any bits on entry
      ULONG_MAX,                      // Clear all bits on exit
      &taskNotification,              // Receives the notification value
      pdMS_TO_TICKS(1000));
    if (notified == pdPASS) {
      if (taskNotification & X_CMD_DATA_READY) {
        taskNotification &= ~X_CMD_DATA_READY;
        instance->readScd30();
      }
    } else {
      if (digitalRead(SCD30_RDY_PIN)) {
        ESP_LOGD(TAG, "Missed Interrupt - updating out of band");
        instance->readScd30();
      }
    }
  }
  vTaskDelete(NULL);
}