#include "globals.h"
#include <config.h>
#include <sps_30.h>
#include <model.h>

#include <i2c.h>
#include <configManager.h>

#define SP30_COMMS Wire

boolean SPS_30::checkError(uint16_t error, char const* msg) {
  if (error != ERR__OK) {
    ESP_LOGW(TAG, "Error trying to execute %s: %x", msg, error);
    I2C::giveMutex();
    this->updateMessageCallback("error SPS30 cmd");
    while (!I2C::takeMutex(pdMS_TO_TICKS(portMAX_DELAY)));
    return false;
  }
  return true;
}

SPS_30::SPS_30(TwoWire* wire, Model* _model, updateMessageCallback_t _updateMessageCallback) {
  this->model = _model;
  this->updateMessageCallback = _updateMessageCallback;
  this->sps30 = new SPS30();
  ESP_LOGD(TAG, "Initialising SPS30");

  sps30->EnableDebugging(2);

  if (!I2C::takeMutex(pdMS_TO_TICKS(portMAX_DELAY))) return;


  if (sps30->begin(*wire) == false) {
    this->updateMessageCallback("SPS30 fail");
    ESP_LOGD(TAG, "Could not initialise SPS30!");
  }
  I2C::giveMutex();
  ESP_LOGD(TAG, "SPS30 initialised");
}

SPS_30::~SPS_30() {
  if (this->task) vTaskDelete(this->task);
  if (this->sps30) delete sps30;
}

boolean SPS_30::readSps30() {
  /*
  this->updateMessageCallback("readSps30");

  // check if data is ready
  uint16_t dataReady;
  if (!I2C::takeMutex(pdMS_TO_TICKS(1000))) return false;
  I2C::giveMutex();

  ESP_LOGD(TAG, "Temp: %.1fC, rH: %.1f%%, CO2:  %uppm", temperature, humidity, co2);
  this->updateMessageCallback("");
  model->updateModel(co2, temperature, humidity);
  if (co2 == 0) {
    ESP_LOGW(TAG, "Invalid sample detected, skipping.");
    this->updateMessageCallback("Invalid sample");
  } else {
    return true;
  }*/
  return false;
}

TaskHandle_t SPS_30::start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core) {
  xTaskCreatePinnedToCore(
    this->sps30Loop,  // task function
    name,             // name of task
    stackSize,        // stack size of task
    this,             // parameter of the task
    priority,         // priority of the task
    &task,            // task handle
    core);            // CPU core
  return this->task;
}

void SPS_30::sps30Loop(void* pvParameters) {
  SPS_30* instance = (SPS_30*)pvParameters;

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(5000));
    instance->readSps30();
  }
  vTaskDelete(NULL);
}