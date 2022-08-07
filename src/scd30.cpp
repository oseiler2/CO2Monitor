#include "globals.h"
#include <config.h>
#include <scd30.h>
#include <Arduino.h>

#include <configManager.h>
#include <i2c.h>
#include <esp32-hal-timer.h>
#include "freertos/FreeRTOS.h"

// Local logging tag
static const char TAG[] = __FILE__;

#define MAX_RETRY 5
#define SCD30_INTERVAL 15

SCD30::SCD30(TwoWire* wire, Model* _model, updateMessageCallback_t _updateMessageCallback) {
  this->model = _model;
  this->updateMessageCallback = _updateMessageCallback;
  this->scd30 = new Adafruit_SCD30();

  if (!I2C::takeMutex(portMAX_DELAY)) return;
  Wire.setClock(SCD30_I2C_CLK);

  uint8_t retry = 0;
  while (retry < MAX_RETRY && !scd30->begin(SCD30_I2CADDR_DEFAULT, wire, 0)) retry++;
  if (retry >= MAX_RETRY) {
    ESP_LOGW(TAG, "Failed to find SCD30 chip");
  }

  retry = 0;
  while (retry < MAX_RETRY && !scd30->setMeasurementInterval(SCD30_INTERVAL)) retry++;
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
  Wire.setClock(I2C_CLK);
  I2C::giveMutex();
  initialised = true;
  ESP_LOGD(TAG, "SCD30 initialised");
}

SCD30::~SCD30() {
  if (this->scd30) delete scd30;
}

uint32_t SCD30::getInterval() {
  return SCD30_INTERVAL;
}

boolean SCD30::readScd30() {
#ifdef SHOW_DEBUG_MSGS
  this->updateMessageCallback("readScd30");
#endif
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return false;
  Wire.setClock(SCD30_I2C_CLK);
  boolean read = scd30->dataReady() && scd30->read();
  Wire.setClock(I2C_CLK);
  I2C::giveMutex();
  if (read) {
    ESP_LOGD(TAG, "Temp: %.1fC, rH: %.1f%%, CO2:  %.0fppm", scd30->temperature, scd30->relative_humidity, scd30->CO2);
#ifdef SHOW_DEBUG_MSGS
    updateMessageCallback("");
#endif
    model->updateModel(scd30->CO2, scd30->temperature, scd30->relative_humidity);
    return true;
  } else {
#ifdef SHOW_DEBUG_MSGS
    updateMessageCallback("sensor read error");
#endif
    ESP_LOGW(TAG, "Error reading sensor data");
  }
  return false;
}

boolean SCD30::calibrateScd30ToReference(uint16_t co2Reference) {
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return false;
  Wire.setClock(SCD30_I2C_CLK);
  uint8_t retry = 0;
  while (retry++ < MAX_RETRY && !scd30->forceRecalibrationWithReference(co2Reference));
  ESP_LOGD(TAG, "co2Reference: %u, result %s", co2Reference, (retry < MAX_RETRY) ? "true" : "false");
  Wire.setClock(I2C_CLK);
  I2C::giveMutex();
  return (retry < MAX_RETRY);
}

float SCD30::getTemperatureOffset() {
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return false;
  Wire.setClock(SCD30_I2C_CLK);
  float temperatureOffset = scd30->getTemperatureOffset() / 100.0;
  ESP_LOGD(TAG, "Temperature offset: %.1f C", temperatureOffset);
  Wire.setClock(I2C_CLK);
  I2C::giveMutex();
  return temperatureOffset;
}

boolean SCD30::setTemperatureOffset(float temperatureOffset) {
  if (temperatureOffset < 0) {
    ESP_LOGW(TAG, "Negative temperature offset not supported");
    return false;
  }
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return false;
  Wire.setClock(SCD30_I2C_CLK);
  uint8_t retry = 0;
  while (retry < MAX_RETRY && !scd30->setTemperatureOffset(floor(temperatureOffset * 100))) retry++;
  if (retry >= MAX_RETRY)
    ESP_LOGW(TAG, "Failed to set temperature offset");
  Wire.setClock(I2C_CLK);
  I2C::giveMutex();
  return (retry < MAX_RETRY);
}

boolean SCD30::setAmbientPressure(uint16_t ambientPressureInHpa) {
  if (!initialised) return false;
  if (ambientPressureInHpa == lastAmbientPressure) return true;
  lastAmbientPressure = ambientPressureInHpa;
  ESP_LOGD(TAG, "setAmbientPressure: %u", ambientPressureInHpa);
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return false;
  Wire.setClock(SCD30_I2C_CLK);
  boolean success = scd30->startContinuousMeasurement(ambientPressureInHpa);
  if (!success) {
    ESP_LOGD(TAG, "failed to setAmbientPressure");
  }
  Wire.setClock(I2C_CLK);
  I2C::giveMutex();
  return success;
}
