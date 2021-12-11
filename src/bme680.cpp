#include "globals.h"
#include <config.h>
#include <bme680.h>
#include <model.h>

#include <i2c.h>
#include <configManager.h>

const uint8_t bsec_config_iaq[] = {
#include "config/generic_33v_3s_4d/bsec_iaq.txt"
};

void BME680::checkIaqSensorStatus() {
  if (bme680->status != BSEC_OK) {
    if (bme680->status < BSEC_OK) {
      ESP_LOGW(TAG, "BSEC error code: %s", String(bme680->status));
    } else {
      ESP_LOGW(TAG, "BSEC warning code: %s", String(bme680->status));
    }
  }

  if (bme680->bme680Status != BME680_OK) {
    if (bme680->bme680Status < BME680_OK) {
      ESP_LOGW(TAG, "BME680 error code: %s", String(bme680->bme680Status));
    } else {
      ESP_LOGW(TAG, "BME680 warning code: %s", String(bme680->bme680Status));
    }
  }
}

const float SAMPLE_RATE = BSEC_SAMPLE_RATE_CONTINUOUS;

BME680::BME680(TwoWire* wire, Model* _model, updateMessageCallback_t _updateMessageCallback) {
  this->model = _model;
  this->updateMessageCallback = _updateMessageCallback;
  this->bme680 = new Bsec();
  ESP_LOGD(TAG, "Initialising BME680");

  if (!I2C::takeMutex(pdMS_TO_TICKS(portMAX_DELAY))) return;

  bme680->begin(BME680_I2C_ADDR_SECONDARY, *wire);

  checkIaqSensorStatus();

  bme680->setConfig(bsec_config_iaq);
  checkIaqSensorStatus();

  bsec_virtual_sensor_t sensorList[14] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_COMPENSATED_GAS,
    BSEC_OUTPUT_GAS_PERCENTAGE,
  };

  bme680->updateSubscription(sensorList, 14, SAMPLE_RATE);
  checkIaqSensorStatus();

  /*
    bme680->setTemperatureOversampling(BME680_OS_8X);
    bme680->setHumidityOversampling(BME680_OS_2X);
    bme680->setPressureOversampling(BME680_OS_4X);
    bme680->setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme680->setGasHeater(320, 150); // 320*C for 150 ms
  */
  I2C::giveMutex();
  ESP_LOGD(TAG, "BME680 initialised");
}

BME680::~BME680() {
  if (this->task) vTaskDelete(this->task);
  if (this->bme680) delete bme680;
}


boolean BME680::readBme680() {
  this->updateMessageCallback("readBme680");
  if (!I2C::takeMutex(pdMS_TO_TICKS(1000))) return false;
  boolean run = bme680->run();
  I2C::giveMutex();
  if (run) { // If new data is available
    ESP_LOGD(TAG, "Temperature: %.1f C (raw %.1f C)", bme680->temperature, bme680->rawTemperature);
    ESP_LOGD(TAG, "Humidity: %.1f %% (raw %.1f %%)", bme680->humidity, bme680->rawHumidity);
    ESP_LOGD(TAG, "Pressure: %.1f hPa", bme680->pressure / 100);
    ESP_LOGD(TAG, "Gas Resistance: %.1f kOhm", bme680->gasResistance / 1000);
    ESP_LOGD(TAG, "Comp gas Value: %.1f, accuracy: %u", bme680->compGasValue, bme680->compGasAccuracy);
    ESP_LOGD(TAG, "Gas percentage: %.1f, accuracy: %u", bme680->gasPercentage, bme680->gasPercentageAcccuracy);
    ESP_LOGD(TAG, "IAQ: %.1f, accuracy: %u", bme680->iaq, bme680->iaqAccuracy);
    ESP_LOGD(TAG, "Static IAQ: %.1f, accuracy: %u", bme680->staticIaq, bme680->staticIaqAccuracy);
    ESP_LOGD(TAG, "CO2 equiv: %.1f, accuracy: %u", bme680->co2Equivalent, bme680->co2Accuracy);
    ESP_LOGD(TAG, "Breath Voc equiv: %.1f, accuracy: %u", bme680->breathVocEquivalent, bme680->breathVocAccuracy);
    ESP_LOGD(TAG, "Run in status: %.1f, Stab status: %.1f", bme680->runInStatus, bme680->stabStatus);
    updateMessageCallback("");
    //    model->updateModel(model->getCo2(), bme680->temperature, bme680->humidity);
  } else {
    checkIaqSensorStatus();
    this->updateMessageCallback("");
    return false;
  }
  return true;
}

TaskHandle_t BME680::start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core) {
  xTaskCreatePinnedToCore(
    this->bme680Loop,  // task function
    name,             // name of task
    stackSize,        // stack size of task
    this,             // parameter of the task
    priority,         // priority of the task
    &task,            // task handle
    core);            // CPU core
  return this->task;
}

void BME680::bme680Loop(void* pvParameters) {
  BME680* instance = (BME680*)pvParameters;

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1 / SAMPLE_RATE * 1000));
    instance->readBme680();
  }
  vTaskDelete(NULL);
}