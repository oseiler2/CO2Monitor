#include "globals.h"
#include <config.h>
#include <scd40.h>
#include <model.h>

#include <i2c.h>
#include <configManager.h>

// Local logging tag
static const char TAG[] = __FILE__;

boolean SCD40::checkError(uint16_t error, char const* msg) {
  if (error != 0) {
    char errorMessage[256];
    errorToString(error, errorMessage, 256);
    ESP_LOGW(TAG, "Error trying to execute %s: %s", msg, errorMessage);
    I2C::giveMutex();
    this->updateMessageCallback("error SCD40 cmd");
    while (!I2C::takeMutex(pdMS_TO_TICKS(portMAX_DELAY)));
    return false;
  }
  return true;
}

SCD40::SCD40(TwoWire* wire, Model* _model, updateMessageCallback_t _updateMessageCallback) {
  this->model = _model;
  this->updateMessageCallback = _updateMessageCallback;
  this->scd40 = new SensirionI2CScd4x();
  ESP_LOGD(TAG, "Initialising SCD40");

  if (!I2C::takeMutex(pdMS_TO_TICKS(portMAX_DELAY))) return;

  scd40->begin(*wire);

  // stop potentially previously started measurement
  checkError(scd40->stopPeriodicMeasurement(), "stopPeriodicMeasurement");

  vTaskDelay(pdMS_TO_TICKS(500));

  /*
    if (checkError(scd40->setTemperatureOffset(7.0f), "setTemperatureOffset")) {
      ESP_LOGD(TAG, "Temperature offset: %.1f", 7.0);
      checkError(scd40->persistSettings(), "persistSettings");
    }
  */

  /*
    uint16_t sensorStatus;
    ESP_LOGD(TAG, "Performing sensor self test...");
    if (checkError(scd40->performSelfTest(sensorStatus), "performSelfTest")) {
      if (sensorStatus != 0) {
        ESP_LOGW(TAG, "Self check error: %x", sensorStatus);
        //      checkError(scd40->performFactoryReset(), "performFactoryReset");
      }
    }
  */

  uint16_t serialNo[3];
  if (checkError(scd40->getSerialNumber(serialNo[0], serialNo[1], serialNo[2]), "getSerialNumber")) {
    ESP_LOGD(TAG, "SCD40 serial#: %x%x%x", serialNo[0], serialNo[1], serialNo[2]);
  }

  uint16_t ascEnabled;
  if (checkError(scd40->getAutomaticSelfCalibration(ascEnabled), "getAutomaticSelfCalibration")) {
    ESP_LOGD(TAG, "Automatic self-calibration: %u", ascEnabled);
    if (ascEnabled != 1) {
      checkError(scd40->setAutomaticSelfCalibration(0x01), "setAutomaticSelfCalibration");
    }
  }

  uint16_t sensor_altitude;
  if (checkError(scd40->getSensorAltitude(sensor_altitude), "getSensorAltitude")) {
    if (sensor_altitude != config.altitude) {
      checkError(scd40->setSensorAltitude(config.altitude), "setSensorAltitude");
      checkError(scd40->persistSettings(), "persistSettings");
    }
  }

  float temperature_offset;
  if (checkError(scd40->getTemperatureOffset(temperature_offset), "getTemperatureOffset")) {
    ESP_LOGD(TAG, "Temperature offset: %.1f", temperature_offset);
  }

  // Start Measurement
  checkError(scd40->startPeriodicMeasurement(), "startPeriodicMeasurement");
  I2C::giveMutex();
  ESP_LOGD(TAG, "SCD40 initialised");
}

SCD40::~SCD40() {
  if (this->task) vTaskDelete(this->task);
  if (this->scd40) delete scd40;
}


boolean SCD40::readScd40() {
  // check if data is ready
  uint16_t dataReady;
  if (!I2C::takeMutex(pdMS_TO_TICKS(1000))) return false;
  boolean success = checkError(scd40->getDataReadyStatus(dataReady), "getDataReadyStatus");
  I2C::giveMutex();
  if (!success) return false;
  if ((dataReady & 0x07ff) == 0) {
    this->updateMessageCallback("SCD40 not ready");
    ESP_LOGD(TAG, "SCD40 measurement not ready! (%x)", dataReady);
    return false;
  }

  float temperature, humidity = NaN;
  uint16_t co2 = 0x0000u;
  // Read Measurement
  if (!I2C::takeMutex(pdMS_TO_TICKS(1000))) return false;
  success = checkError(scd40->readMeasurement(co2, temperature, humidity), "readMeasurement");
  I2C::giveMutex();
  if (!success) return false;
  ESP_LOGD(TAG, "Temp: %.1fC, rH: %.1f%%, CO2:  %uppm", temperature, humidity, co2);
  this->updateMessageCallback("");
  model->updateModel(co2, temperature, humidity);
  if (co2 == 0) {
    ESP_LOGW(TAG, "Invalid sample detected, skipping.");
    this->updateMessageCallback("Invalid sample");
  } else {
    return true;
  }
  return false;
}

boolean SCD40::calibrateScd40ToReference(uint16_t co2Reference) {
  if (!I2C::takeMutex(pdMS_TO_TICKS(1000))) return false;
  boolean success = checkError(scd40->stopPeriodicMeasurement(), "stopPeriodicMeasurement");
  if (!success) {
    I2C::giveMutex();
    ESP_LOGD(TAG, "failed to calibrateScd40ToReference");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(500));
  uint16_t frcCorrection;
  success = checkError(scd40->performForcedRecalibration(co2Reference, frcCorrection), "performForcedRecalibration");
  if (!success) {
    I2C::giveMutex();
    ESP_LOGD(TAG, "failed to calibrateScd40ToReference");
    return false;
  }
  ESP_LOGD(TAG, "co2Reference: %u, frcCorrection %u", co2Reference, frcCorrection);
  success = checkError(scd40->startPeriodicMeasurement(), "startPeriodicMeasurement");
  I2C::giveMutex();
  return success;
}

float SCD40::getTemperatureOffset() {
  if (!I2C::takeMutex(pdMS_TO_TICKS(1000))) return false;
  boolean success = checkError(scd40->stopPeriodicMeasurement(), "stopPeriodicMeasurement");
  if (!success) {
    I2C::giveMutex();
    ESP_LOGD(TAG, "failed to getTemperatureOffset");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(500));
  float temperatureOffset;
  success = checkError(scd40->getTemperatureOffset(temperatureOffset), "getTemperatureOffset");
  if (!success) {
    I2C::giveMutex();
    ESP_LOGD(TAG, "failed to getTemperatureOffset");
    return false;
  }
  ESP_LOGD(TAG, "getTemperatureOffset: %.1f", temperatureOffset);
  success = checkError(scd40->startPeriodicMeasurement(), "startPeriodicMeasurement");
  I2C::giveMutex();
  return temperatureOffset;
}

boolean SCD40::setTemperatureOffset(float temperatureOffset) {
  if (temperatureOffset < 0) {
    ESP_LOGW(TAG, "Negative temperature offset not supported");
    return false;
  }
  if (!I2C::takeMutex(pdMS_TO_TICKS(1000))) return false;
  boolean success = checkError(scd40->stopPeriodicMeasurement(), "stopPeriodicMeasurement");
  if (!success) {
    I2C::giveMutex();
    ESP_LOGD(TAG, "failed to setTemperatureOffset");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(500));
  success = checkError(scd40->setTemperatureOffset(temperatureOffset), "setTemperatureOffset");
  if (!success) {
    ESP_LOGD(TAG, "failed to setTemperatureOffset");
  } else {
    success = checkError(scd40->persistSettings(), "persistSettings");
    if (!success) {
      ESP_LOGD(TAG, "failed to persist temperatureOffset");
    }
  }
  ESP_LOGD(TAG, "setTemperatureOffset: %.1f", temperatureOffset);
  success = checkError(scd40->startPeriodicMeasurement(), "startPeriodicMeasurement");
  I2C::giveMutex();
  return success;
}

TaskHandle_t SCD40::start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core) {
  xTaskCreatePinnedToCore(
    this->scd40Loop,  // task function
    name,             // name of task
    stackSize,        // stack size of task
    this,             // parameter of the task
    priority,         // priority of the task
    &task,            // task handle
    core);            // CPU core
  return this->task;
}

void SCD40::scd40Loop(void* pvParameters) {
  SCD40* instance = (SCD40*)pvParameters;

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(5000));
    instance->readScd40();
  }
  vTaskDelete(NULL);
}
