#include <config.h>
#include <configManager.h>
#include <battery.h>
#include <driver/adc.h>

// Local logging tag
static const char TAG[] = "Battery";

namespace Battery {

#if HAS_BATTERY
  Model* model;

  void init(Model* _model) {
    model = _model;
    adcAttachPin(VBAT_ADC);
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
    digitalWrite(VBAT_EN, HIGH);
    adc_power_acquire();
    uint32_t maxValue = 0;
    uint32_t minValue = 0xffffffff;
    uint32_t sumValue = 0;
    uint32_t reading;
#define NUMBER_OF_READINGS         10
    for (uint8_t i = 0; i < NUMBER_OF_READINGS; i++) {
      reading = analogReadMilliVolts(VBAT_ADC);
      sumValue += reading;
      if (reading > maxValue) maxValue = reading;
      if (reading < minValue) minValue = reading;
    }
    uint16_t mV_raw = (sumValue - maxValue - minValue) / (NUMBER_OF_READINGS - 2);
    digitalWrite(VBAT_EN, LOW);
    uint16_t mV = mV_raw * 1.496f; //* 1.365f;  //1.496f;  (1k vs 1k5)
    adc_power_release();
    ESP_LOGI(TAG, "Battery: %u mV (raw: %u mV)", mV, mV_raw);
    return mV;
  }
#else

  void init(Model* _model) {}
  void readVoltage() {}
  boolean batteryPresent() { return false; }
  boolean usbPowerPresent() { return true; }
  uint16_t getBatteryLevelInmV() { return 4200; }
  uint8_t getBatteryLevelInPercent(uint16_t mV) { return 100; }

#endif

}