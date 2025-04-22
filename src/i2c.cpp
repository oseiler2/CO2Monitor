#include <i2c.h>
#include <Wire.h>
#include <config.h>

// Local logging tag
static const char TAG[] = "I2C";

typedef enum {
  BM_UNKNOWN = 0,
  BM_BME280,
  BM_BME680
} BmSensorType;

namespace I2C {
  boolean lcdDetected = false;
  boolean scd30Detected = false;
  boolean scd40Detected = false;
  boolean sps30Detected = false;
  boolean bme280Detected = false;
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

  boolean bme280Present() {
    return bme280Detected;
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

  BmSensorType checkBmType(uint8_t addr) {
    Wire.beginTransmission(addr);
    Wire.write(0xD0);
    Wire.endTransmission();
    delay(20);
    Wire.requestFrom((uint8_t)addr, 1u);
    uint16_t reg = 0x0000u;
    if (Wire.available() > 1) {
      // Read MSB, then LSB
      reg = (uint16_t)Wire.read() << 8;
      reg |= Wire.read();
    } else if (Wire.available()) {
      reg = Wire.read();
    }
    switch (reg) {
      case 0x60:
        ESP_LOGD(TAG, "BME280 found");
        return BM_BME280;
      case 0x61:
        ESP_LOGD(TAG, "BME680 found");
        return BM_BME680;
      default:
        ESP_LOGW(TAG, "Unknown device at %02x found reg: %02x", addr, reg);
        return BM_UNKNOWN;
    }
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
          } else if (addr == BMx_I2C_ADR) {
            BmSensorType t = checkBmType(addr);
            switch (t) {
              case BM_BME280:
                bme280Detected = true;
                break;
              case BM_BME680:
                bme680Detected = true;
                break;
              default:
                ESP_LOGD(TAG, "Unknown device at %02x found", BMx_I2C_ADR);
                break;
            }
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
      Wire.beginTransmission(BMx_I2C_ADR);
      if (Wire.endTransmission() == 0) {
        BmSensorType t = checkBmType(addr);
        switch (t) {
          case BM_BME280:
            nDevices++;
            bme280Detected = true;
            break;
          case BM_BME680:
            nDevices++;
            bme680Detected = true;
            break;
          default:
            ESP_LOGD(TAG, "Unknown device at %02x found", BMx_I2C_ADR);
            break;
        }
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