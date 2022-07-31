#include "globals.h"
#include <config.h>
#include <sps_30.h>
#include <model.h>

#include <i2c.h>
#include <configManager.h>

// Local logging tag
static const char TAG[] = __FILE__;

#define SP30_COMMS Wire

boolean SPS_30::checkError(uint16_t error, char const* msg) {
  if (error != SPS30_ERR_OK) {
    ESP_LOGW(TAG, "Error trying to execute %s: %x", msg, error);
    I2C::giveMutex();
#ifdef SHOW_DEBUG_MSGS
    this->updateMessageCallback("error SPS30 cmd");
#endif
    while (!I2C::takeMutex(portMAX_DELAY));
    return false;
  }
  return true;
}

SPS_30::SPS_30(TwoWire* wire, Model* _model, updateMessageCallback_t _updateMessageCallback) {
  this->model = _model;
  this->updateMessageCallback = _updateMessageCallback;
  this->sps30 = new SPS30();
  ESP_LOGD(TAG, "Initialising SPS30");

  //  sps30->EnableDebugging(2);

  if (!I2C::takeMutex(portMAX_DELAY)) return;

  if (sps30->begin(wire) == false) {
    ESP_LOGD(TAG, "Could not initialise SPS30!");
    I2C::giveMutex();
#ifdef SHOW_DEBUG_MSGS
    this->updateMessageCallback("SPS30 fail");
#endif
    return;
  }
  if (!sps30->probe()) {
    ESP_LOGD(TAG, "Could not probe SPS30!");
    I2C::giveMutex();
#ifdef SHOW_DEBUG_MSGS
    this->updateMessageCallback("SPS30 fail");
#endif
    return;
  }
  SPS30_version version;
  if (sps30->GetVersion(&version) == SPS30_ERR_OK) {
    ESP_LOGD(TAG, "SPS30 version: %u.%u", version.major, version.minor);
  } else {
    ESP_LOGI(TAG, "Could not get SPS30 version!");
  }
  uint32_t autoCleanInterval = 0;
  if (sps30->GetAutoCleanInt(&autoCleanInterval) == SPS30_ERR_OK) {
    ESP_LOGD(TAG, "SPS30 auto clean interval: %u days %u hours %u minutes %u seconds (%u)", autoCleanInterval / 60 / 60 / 24, autoCleanInterval / 60 / 60 % 24, autoCleanInterval / 60 % 60, autoCleanInterval % 60, autoCleanInterval);
  } else {
    ESP_LOGI(TAG, "Could not get auto clean interval!");
  }
  I2C::giveMutex();
  ESP_LOGD(TAG, "SPS30 initialised");
}

SPS_30::~SPS_30() {
  if (this->sps30) delete sps30;
}

uint32_t SPS_30::getInterval() {
  return 60;
}

boolean SPS_30::readSps30() {
  //  ESP_LOGD(TAG, "readSps30");
#ifdef SHOW_DEBUG_MSGS
  this->updateMessageCallback("readSps30");
#endif
  struct sps_values values;

  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return false;
  if (!sps30->start()) {
    ESP_LOGD(TAG, "Could not start SPS30!");
    I2C::giveMutex();
#ifdef SHOW_DEBUG_MSGS
    this->updateMessageCallback("SPS30 start fail");
#endif
    return false;
  }

  I2C::giveMutex();
  delay(5000);
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return false;

  uint8_t result = SPS30_ERR_TIMEOUT;
  for (int i = 0;i < 3 && result == SPS30_ERR_TIMEOUT;i++) {
    result = sps30->GetValues(&values);
  }

  if (!sps30->stop()) {
    ESP_LOGD(TAG, "Could not stop SPS30!");
    I2C::giveMutex();
#ifdef SHOW_DEBUG_MSGS
    this->updateMessageCallback("SPS30 stop fail");
#endif
    return false;
  }

  I2C::giveMutex();
#ifdef SHOW_DEBUG_MSGS
  this->updateMessageCallback("");
#endif
  if (result == SPS30_ERR_OK) {
    ESP_LOGD(TAG, "SPS30 MassPM1:%.1f, MassPM2:%.1f, MassPM4:%.1f, MassPM10:%.1f, NumPM0:%.1f, NumPM1:%.1f, NumPM2:%.1f, NumPM4:%.1f, NumPM10:%.1f, PartSize:%.1f",
      values.MassPM1, values.MassPM2, values.MassPM4, values.MassPM10, values.NumPM0, values.NumPM1, values.NumPM2, values.NumPM4, values.NumPM10, values.PartSize);
    model->updateModel((uint16_t)(values.NumPM0 + 0.5f), (uint16_t)(values.NumPM1 + 0.5f), (uint16_t)(values.NumPM2 + 0.5f), (uint16_t)(values.NumPM4 + 0.5f), (uint16_t)(values.NumPM10 + 0.5f));
  }
  //  ESP_LOGD(TAG, "Sps30 done");
  return (result == SPS30_ERR_OK);
}

uint8_t SPS_30::getStatus() {
  ESP_LOGD(TAG, "getStatus");
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return 0xff;
  uint8_t value = 0;
  uint8_t result = sps30->GetStatusReg(&value);
  I2C::giveMutex();
  if (result != SPS30_ERR_OK) return 0xff;
  return value;
}

uint32_t SPS_30::getAutoCleanInterval() {
  ESP_LOGD(TAG, "getAutoCleanInterval");
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return 0xffffffff;
  uint32_t value = 0;
  uint8_t result = sps30->GetAutoCleanInt(&value);
  I2C::giveMutex();
  if (result != SPS30_ERR_OK) return 0xffffffff;
  return value;
}

boolean SPS_30::setAutoCleanInterval(uint32_t intervalInSeconds) {
  ESP_LOGD(TAG, "setAutoCleanInterval %u", intervalInSeconds);
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return false;
  uint8_t result = sps30->SetAutoCleanInt(intervalInSeconds);
  I2C::giveMutex();
  return (result == SPS30_ERR_OK);
}

boolean SPS_30::clean() {
  ESP_LOGD(TAG, "clean");
#ifdef SHOW_DEBUG_MSGS
  this->updateMessageCallback("clean sps30");
#endif
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return false;

  if (!sps30->start()) {
    ESP_LOGD(TAG, "Could not start SPS30!");
    I2C::giveMutex();
#ifdef SHOW_DEBUG_MSGS
    this->updateMessageCallback("SPS30 start fail");
#endif
    return false;
  }
  I2C::giveMutex();
  delay(5000);
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return false;
  boolean result = sps30->clean();
  if (!sps30->stop()) {
    ESP_LOGD(TAG, "Could not stop SPS30!");
    I2C::giveMutex();
#ifdef SHOW_DEBUG_MSGS
    this->updateMessageCallback("SPS30 stop fail");
#endif
    return false;
  }
  I2C::giveMutex();
#ifdef SHOW_DEBUG_MSGS
  this->updateMessageCallback("");
#endif
  return result;
}
