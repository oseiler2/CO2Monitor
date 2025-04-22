#include <bme280.h>

#include <i2c.h>
#include <configManager.h>
#include <power.h>

// Local logging tag
static const char TAG[] = "BME280";

BME280::BME280(TwoWire* wire, Model* _model, updateMessageCallback_t _updateMessageCallback, boolean reinitFromSleep) {
  this->model = _model;
  this->updateMessageCallback = _updateMessageCallback;
  this->bme280 = new Adafruit_BME280();
  ESP_LOGD(TAG, "Initialising BME280");

  if (!I2C::takeMutex(portMAX_DELAY)) return;

  if (!bme280->begin(BMx_I2C_ADR, wire)) {
    ESP_LOGW(TAG, "Could not initialise BME280");
    I2C::giveMutex();
    ESP_LOGD(TAG, "BME280 initialised");
  }

  ESP_LOGD(TAG, "BME280 sensorId: %08x", bme280->sensorID());

  if (true || Power::getRunMode() == RM_LOW) {
    this->sampleRate = 1.0f / 30;
    this->bme280->setSampling(Adafruit_BME280::MODE_FORCED,
      Adafruit_BME280::SAMPLING_X2, // Temp. oversampling
      Adafruit_BME280::SAMPLING_X2, // Pressure oversampling
      Adafruit_BME280::SAMPLING_X2, // Humidity oversampling
      Adafruit_BME280::FILTER_X2,
      Adafruit_BME280::STANDBY_MS_1000);
  } else {
    this->sampleRate = 1.0f;
    this->bme280->setSampling(Adafruit_BME280::MODE_NORMAL,
      Adafruit_BME280::SAMPLING_X16, // Temp. oversampling
      Adafruit_BME280::SAMPLING_X16, // Pressure oversampling
      Adafruit_BME280::SAMPLING_X16, // Humidity oversampling
      Adafruit_BME280::FILTER_X16,
      Adafruit_BME280::STANDBY_MS_1000);

  }

  if (!reinitFromSleep || Power::getRunMode() == RM_FULL) {
    //    bme280->setTemperatureOffset(7.0);
  }

  I2C::giveMutex();
  ESP_LOGD(TAG, "BME280 initialised");
}

BME280::~BME280() {
  if (this->bme280) delete bme280;
}

void BME280::shutdown() {
  if (this->bme280) {
    if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return;
    this->bme280->setSampling(Adafruit_BME280::MODE_SLEEP);
  }
  I2C::giveMutex();
}

uint32_t BME280::getInterval() {
  return floor(1 / this->sampleRate);
}

boolean BME280::readBme280() {
//  ESP_LOGD(TAG, "BME280::readBme280()");
#ifdef SHOW_DEBUG_MSGS
  this->updateMessageCallback("readBme280");
#endif
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return false;
  if (sampleRate < 1.0f) {
    if (!bme280->takeForcedMeasurement()) {
      ESP_LOGI(TAG, "takeForcedMeasurement returned false");
      I2C::giveMutex();
      return false;
    }
  }
  float temp = bme280->readTemperature();
  float pressure = bme280->readPressure();
  float humidity = bme280->readHumidity();
  I2C::giveMutex();
  ESP_LOGD(TAG, "Temp: %.1fC, Hum: %.1f%%, Pressure: %.1fhPa", temp, humidity, pressure / 100);
#ifdef SHOW_DEBUG_MSGS
  updateMessageCallback("");
#endif
  if (pressure > 0 && humidity > 0) model->updateModelTHP(temp, humidity, (uint16_t)(pressure / 100));
  return true;
}

boolean BME280::setSampleRate(float _sampleRate) {
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return false;
  if (_sampleRate == 1.0f) {
    ESP_LOGD(TAG, "BME280::STANDBY_MS_1000");
    this->sampleRate = 1.0f;
    this->bme280->setSampling(Adafruit_BME280::MODE_NORMAL,
      Adafruit_BME280::SAMPLING_X16, // Temp. oversampling
      Adafruit_BME280::SAMPLING_X16, // Pressure oversampling
      Adafruit_BME280::SAMPLING_X16, // Humidity oversampling
      Adafruit_BME280::FILTER_X16,
      Adafruit_BME280::STANDBY_MS_1000);
  } else if (_sampleRate < 1.0f) {
    ESP_LOGD(TAG, "BME280::MODE_FORCED");
    this->sampleRate = 1.0f / 30;
    this->bme280->setSampling(Adafruit_BME280::MODE_FORCED,
      Adafruit_BME280::SAMPLING_X2, // Temp. oversampling
      Adafruit_BME280::SAMPLING_X2, // Pressure oversampling
      Adafruit_BME280::SAMPLING_X2, // Humidity oversampling
      Adafruit_BME280::FILTER_X2,
      Adafruit_BME280::STANDBY_MS_1000);
  } else {
    ESP_LOGW(TAG, "invalid sample rate: %.2f", _sampleRate);
  }
  I2C::giveMutex();
  ESP_LOGD(TAG, "BME280::setSampleRate - done");
  return true;
}

