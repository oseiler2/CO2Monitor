#include <i2c.h>
#include <Wire.h>
#include <config.h>

// Local logging tag
static const char TAG[] = "I2C";

namespace I2C {
  boolean lcdDetected = false;
  boolean scd30Detected = false;
  boolean scd40Detected = false;
  boolean sps30Detected = false;
  boolean bme680Detected = false;

  boolean lcdPresent() {
    return lcdDetected;
  }

  boolean scd30Present() {
    return scd30Detected;
  }

  boolean scd40Present() {
    return scd40Detected;
  }

  boolean sps30Present() {
    return sps30Detected;
  }

  boolean bme680Present() {
    return bme680Detected;
  }

  static SemaphoreHandle_t i2cMutex = xSemaphoreCreateMutex();

  boolean takeMutex(TickType_t blockTime) {
    //  ESP_LOGD(TAG, "%s attempting to take mutex with blockTime: %u", pcTaskGetTaskName(NULL), blockTime);
    if (i2cMutex == NULL) {
      ESP_LOGD(TAG, "i2cMutex is NULL unsuccessful <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
      return false;
    }
    boolean result = (xSemaphoreTake(i2cMutex, blockTime) == pdTRUE);
    if (!result) ESP_LOGD(TAG, "%s take mutex was: %s", pcTaskGetTaskName(NULL), result ? "successful" : "unsuccessful <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
    return result;
  }

  void giveMutex() {
    xSemaphoreGive(i2cMutex);
  }

  void initI2C(boolean fullScan) {
    if (i2cMutex == NULL) {
      ESP_LOGE(TAG, "Could not create I2C Mutex");
      delay(1000);
      esp_restart();
    }
    if (!takeMutex(portMAX_DELAY)) {
      return;
    }
    Wire.setClock(SCD30_I2C_CLK);
    byte err, addr;
    uint8_t nDevices = 0;
    if (fullScan) {
      for (addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        err = Wire.endTransmission();
        if (err == 0) {
          nDevices++;
          if (addr == SSD1306_I2C_ADR) {
            lcdDetected = true;
            ESP_LOGD(TAG, "SSD1306 display found");
          } else if (addr == SCD30_I2C_ADR) {
            scd30Detected = true;
            ESP_LOGD(TAG, "SDC30 found");
          } else if (addr == SCD40_I2C_ADR) {
            scd40Detected = true;
            ESP_LOGD(TAG, "SDC40 found");
          } else if (addr == SPS30_I2C_ADR) {
            sps30Detected = true;
            ESP_LOGD(TAG, "SPS30 found");
          } else if (addr == BME680_I2C_ADR) {
            bme680Detected = true;
            ESP_LOGD(TAG, "BME680 found");
          } else {
            ESP_LOGI(TAG, "I2C device found at address %x !", addr);
          }
        } else if (err == 4) {
          ESP_LOGW(TAG, "Unknow error at address %x !", addr);
        }
      }
    } else {
      Wire.beginTransmission(SSD1306_I2C_ADR);
      if (Wire.endTransmission() == 0) {
        nDevices++;
        lcdDetected = true;
        ESP_LOGD(TAG, "SSD1306 display found");
      }
      Wire.beginTransmission(SCD30_I2C_ADR);
      if (Wire.endTransmission() == 0) {
        nDevices++;
        scd30Detected = true;
        ESP_LOGD(TAG, "SDC30 found");
      }
      Wire.beginTransmission(SCD40_I2C_ADR);
      if (Wire.endTransmission() == 0) {
        nDevices++;
        scd40Detected = true;
        ESP_LOGD(TAG, "SDC40 found");
      }
      Wire.beginTransmission(SPS30_I2C_ADR);
      if (Wire.endTransmission() == 0) {
        nDevices++;
        sps30Detected = true;
        ESP_LOGD(TAG, "SPS30 found");
      }
      Wire.beginTransmission(BME680_I2C_ADR);
      if (Wire.endTransmission() == 0) {
        nDevices++;
        bme680Detected = true;
        ESP_LOGD(TAG, "BME680 found");
      }
    }
    if (nDevices == 0)
      ESP_LOGD(TAG, "No I2C devices found");
    Wire.setClock(I2C_CLK);
    giveMutex();
  }

  void shutDownI2C() {
    Wire.~TwoWire();
    i2cMutex = NULL;
  }

}