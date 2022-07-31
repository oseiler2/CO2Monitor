#include "globals.h"
#include <config.h>
#include <bme680.h>
#include <model.h>

#include <i2c.h>
#include <configManager.h>
#include <EEPROM.h>

// Local logging tag
static const char TAG[] = __FILE__;

const uint8_t bsec_config_iaq[] = {
#include "config/generic_33v_3s_4d/bsec_iaq.txt"
};

#define STATE_SAVE_PERIOD	UINT32_C(360 * 60 * 1000) // 360 minutes - 4 times a day

uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = { 0 };
uint16_t stateUpdateCounter = 0;

const float SAMPLE_RATE = BSEC_SAMPLE_RATE_LP;

void BME680::loadState(void) {
  if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE) {

    ESP_LOGD(TAG, "Reading state from EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
      bsecState[i] = EEPROM.read(i + 1);
      //      Serial.println(bsecState[i], HEX);
    }

    bme680->setState(bsecState);
    checkIaqSensorStatus();
  } else {
    // Erase the EEPROM with zeroes
    ESP_LOGD(TAG, "Erasing EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE + 1; i++)
      EEPROM.write(i, 0);

    EEPROM.commit();
  }
}

void BME680::updateState(void) {
  bool update = false;
  /* Set a trigger to save the state. Here, the state is saved every STATE_SAVE_PERIOD with the first state being saved once the algorithm achieves full calibration, i.e. iaqAccuracy = 3 */
  if (stateUpdateCounter == 0) {
    if (bme680->iaqAccuracy >= 3) {
      update = true;
      stateUpdateCounter++;
    }
  } else {
    /* Update every STATE_SAVE_PERIOD milliseconds */
    if ((stateUpdateCounter * STATE_SAVE_PERIOD) < millis()) {
      update = true;
      stateUpdateCounter++;
    }
  }

  if (update) {
    bme680->getState(bsecState);
    checkIaqSensorStatus();

    ESP_LOGD(TAG, "Writing state to EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
      EEPROM.write(i + 1, bsecState[i]);
      //      Serial.println(bsecState[i], HEX);
    }

    EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
    EEPROM.commit();
  }
}

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

BME680::BME680(TwoWire* wire, Model* _model, updateMessageCallback_t _updateMessageCallback) {
  this->model = _model;
  this->updateMessageCallback = _updateMessageCallback;
  this->bme680 = new Bsec();
  ESP_LOGD(TAG, "Initialising BME680");

  EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1); // 1st address for the length
  if (!I2C::takeMutex(portMAX_DELAY)) return;

  bme680->begin(BME680_I2C_ADDR_PRIMARY, *wire);
  bme680->setTemperatureOffset(7.0);

  checkIaqSensorStatus();

  bme680->setConfig(bsec_config_iaq);
  checkIaqSensorStatus();

  loadState();

  bsec_virtual_sensor_t sensorList[6] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS,
    //  BSEC_OUTPUT_RAW_TEMPERATURE,
    //  BSEC_OUTPUT_RAW_HUMIDITY,
    //  BSEC_OUTPUT_RAW_GAS,
    //  BSEC_OUTPUT_STATIC_IAQ,  // <--
    //  BSEC_OUTPUT_CO2_EQUIVALENT,
    //  BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    //  BSEC_OUTPUT_COMPENSATED_GAS,
    //  BSEC_OUTPUT_GAS_PERCENTAGE,
  };

  bme680->updateSubscription(sensorList, 6, SAMPLE_RATE);
  checkIaqSensorStatus();

  I2C::giveMutex();
  ESP_LOGD(TAG, "BME680 initialised");
}

BME680::~BME680() {
  if (this->bme680) delete bme680;
}

uint32_t BME680::getInterval() {
  return floor(1 / SAMPLE_RATE);
}

boolean BME680::readBme680() {
#ifdef SHOW_DEBUG_MSGS
  this->updateMessageCallback("readBme680");
#endif
  if (!I2C::takeMutex(I2C_MUTEX_DEF_WAIT)) return false;
  boolean run = bme680->run();
  I2C::giveMutex();
  if (run) { // If new data is available
    ESP_LOGD(TAG, "IAQ: %.1f, acc: %u/%.1f/%.1f, Temp: %.1fC, Hum: %.1f%%, Pressure: %.1fhPa", bme680->iaq, bme680->iaqAccuracy, bme680->runInStatus, bme680->stabStatus, bme680->temperature, bme680->humidity, bme680->pressure / 100);
    //    ESP_LOGD(TAG, "Temperature: %.1f C (raw %.1f C)", bme680->temperature, bme680->rawTemperature);
    //    ESP_LOGD(TAG, "Humidity: %.1f %% (raw %.1f %%)", bme680->humidity, bme680->rawHumidity);
    //    ESP_LOGD(TAG, "Pressure: %.1f hPa", bme680->pressure / 100);
    //    ESP_LOGD(TAG, "Gas Resistance: %.1f kOhm", bme680->gasResistance / 1000);
    //    ESP_LOGD(TAG, "Comp gas Value: %.1f, accuracy: %u", bme680->compGasValue, bme680->compGasAccuracy);
    //    ESP_LOGD(TAG, "Gas percentage: %.1f, accuracy: %u", bme680->gasPercentage, bme680->gasPercentageAcccuracy);
    //    ESP_LOGD(TAG, "IAQ: %.1f, accuracy: %u", bme680->iaq, bme680->iaqAccuracy);
    //    ESP_LOGD(TAG, "Static IAQ: %.1f, accuracy: %u", bme680->staticIaq, bme680->staticIaqAccuracy);
    //    ESP_LOGD(TAG, "CO2 equiv: %.1f, accuracy: %u", bme680->co2Equivalent, bme680->co2Accuracy);
    //    ESP_LOGD(TAG, "Breath Voc equiv: %.1f, accuracy: %u", bme680->breathVocEquivalent, bme680->breathVocAccuracy);
    //    ESP_LOGD(TAG, "Run in status: %.1f, Stab status: %.1f", bme680->runInStatus, bme680->stabStatus);
#ifdef SHOW_DEBUG_MSGS
    updateMessageCallback("");
#endif

    if (bme680->runInStatus && bme680->iaqAccuracy >= 3) {
      model->updateModel(bme680->temperature, bme680->humidity, (uint16_t)(bme680->pressure / 100), (uint16_t)(bme680->iaq));
    } else {
      model->updateModel(bme680->temperature, bme680->humidity, (uint16_t)(bme680->pressure / 100), 0);
    }

    updateState();
  } else {
    checkIaqSensorStatus();
#ifdef SHOW_DEBUG_MSGS
    this->updateMessageCallback("");
#endif
    return false;
  }
  return true;
}
