#include <housekeeping.h>
#include <mqtt.h>
#include <ota.h>

// Local logging tag
static const char TAG[] = __FILE__;

namespace housekeeping {
  Ticker cyclicTimer;

  void doHousekeeping() {
    ESP_LOGI(TAG, "Heap: Free:%u, Min:%u, Size:%u, Alloc:%u, StackHWM:%u",
      ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getHeapSize(),
      ESP.getMaxAllocHeap(), uxTaskGetStackHighWaterMark(NULL));
    ESP_LOGI(TAG, "MqttLoop %u bytes left | Taskstate = %d | core = %u",
      uxTaskGetStackHighWaterMark(mqtt::mqttTask), eTaskGetState(mqtt::mqttTask), xTaskGetAffinity(mqtt::mqttTask));
    ESP_LOGI(TAG, "OtaLoop %u bytes left | Taskstate = %d | core = %u",
      uxTaskGetStackHighWaterMark(OTA::otaTask), eTaskGetState(OTA::otaTask), xTaskGetAffinity(OTA::otaTask));
    if (sensorsTask) {
      ESP_LOGI(TAG, "SensorsLoop %u bytes left | Taskstate = %d | core = %u",
        uxTaskGetStackHighWaterMark(sensorsTask), eTaskGetState(sensorsTask), xTaskGetAffinity(sensorsTask));
    }
    if (neopixelMatrixTask) {
      ESP_LOGI(TAG, "NeopixelMatrixLoop %u bytes left | Taskstate = %d | core = %u",
        uxTaskGetStackHighWaterMark(neopixelMatrixTask), eTaskGetState(neopixelMatrixTask), xTaskGetAffinity(neopixelMatrixTask));
    }
    if (ESP.getMinFreeHeap() <= 2048) {
      ESP_LOGW(TAG,
        "Memory full, counter cleared (heap low water mark = %u Bytes / "
        "free heap = %u bytes)",
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