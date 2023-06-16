#include <Arduino.h>
#include <config.h>
#include <configManager.h>
#include <battery.h>
#include <driver/adc.h>

// Local logging tag
static const char TAG[] = __FILE__;

namespace Battery {

  Model* model;

  void init(Model* _model) {
    model = _model;
    adcAttachPin(config.vBatAdc);
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
  }

  void readVoltage() {
    if (model) model->updateModelmV(getBatteryLevelInmV());
  }

  boolean batteryPresent() {
    if (model && model->getVoltageInMv() > 0) return getBatteryLevelInPercent(model->getVoltageInMv()) > 0;
    return getBatteryLevelInPercent(getBatteryLevelInmV()) > 0;
  }

  boolean usbPowerPresent() {
    if (model && model->getVoltageInMv() > 0) return model->getVoltageInMv() > 4200;
    return getBatteryLevelInmV() > 4200;
  }

  uint8_t getBatteryLevelInPercent(uint16_t mV) {
#define min_mv 3300
#define max_mv 4200
    if (mV <= min_mv) return 0;
    if (mV >= max_mv) return 100;
    return (float)(mV - min_mv) / (max_mv - min_mv) * 100;
  }

  uint16_t getBatteryLevelInmV() {
    digitalWrite(config.vBatEn, HIGH);
    adc_power_acquire();
    uint32_t maxValue = 0;
    uint32_t minValue = 0xffffffff;
    uint32_t sumValue = 0;
    uint32_t reading;
#define NUMBER_OF_READINGS         10
    for (uint8_t i = 0; i < NUMBER_OF_READINGS; i++) {
      reading = analogReadMilliVolts(config.vBatAdc);
      sumValue += reading;
      if (reading > maxValue) maxValue = reading;
      if (reading < minValue) minValue = reading;
    }
    uint16_t mV_raw = (sumValue - maxValue - minValue) / (NUMBER_OF_READINGS - 2);
    digitalWrite(config.vBatEn, LOW);
    uint16_t mV = mV_raw * 1.353f;
    adc_power_release();
    ESP_LOGI(TAG, "Battery: %u mV (raw: %u mV)", mV, mV_raw);
    return mV;
  }

}