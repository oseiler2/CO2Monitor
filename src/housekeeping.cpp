#include <housekeeping.h>
#include <mqtt.h>
#include <i2c.h>
#include <ota.h>

namespace housekeeping {
  Ticker cyclicTimer;

  void doHousekeeping() {
    ESP_LOGD(TAG, "Heap: Free:%d, Min:%d, Size:%d, Alloc:%d, StackHWM:%d",
      ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getHeapSize(),
      ESP.getMaxAllocHeap(), uxTaskGetStackHighWaterMark(NULL));
    ESP_LOGD(TAG, "Mqttloop %d bytes left | Taskstate = %d",
      uxTaskGetStackHighWaterMark(mqtt::mqttTask), eTaskGetState(mqtt::mqttTask));
    ESP_LOGD(TAG, "Otaloop %d bytes left | Taskstate = %d",
      uxTaskGetStackHighWaterMark(OTA::otaTask), eTaskGetState(OTA::otaTask));
    if (I2C::scd30Present() && scd30Task) {
      ESP_LOGD(TAG, "SCD30Loop %d bytes left | Taskstate = %d",
        uxTaskGetStackHighWaterMark(scd30Task), eTaskGetState(scd30Task));
    }
    if (I2C::scd40Present() && scd40Task) {
      ESP_LOGD(TAG, "SCD40Loop %d bytes left | Taskstate = %d",
        uxTaskGetStackHighWaterMark(scd40Task), eTaskGetState(scd40Task));
    }
    if (I2C::sps30Present() && sps30Task) {
      ESP_LOGD(TAG, "SPS30Loop %d bytes left | Taskstate = %d",
        uxTaskGetStackHighWaterMark(sps30Task), eTaskGetState(sps30Task));
    }
    if (I2C::bme680Present() && bme680Task) {
      ESP_LOGD(TAG, "BME680Loop %d bytes left | Taskstate = %d",
        uxTaskGetStackHighWaterMark(bme680Task), eTaskGetState(bme680Task));
    }

    if (ESP.getMinFreeHeap() <= 2048) {
      ESP_LOGW(TAG,
        "Memory full, counter cleared (heap low water mark = %d Bytes / "
        "free heap = %d bytes)",
        ESP.getMinFreeHeap(), ESP.getFreeHeap());
      Serial.flush();
      esp_restart();
    }

  }

  uint32_t getFreeRAM() {
#ifndef BOARD_HAS_PSRAM
    return ESP.getFreeHeap();
#else
    return ESP.getFreePsram();
#endif
  }


}