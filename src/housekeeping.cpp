#include <housekeeping.h>
#include <mqtt.h>
#include <ota.h>
#include <sensors.h>
#include <power.h>
#include <battery.h>
#include <wifiManager.h>

// Local logging tag
static const char TAG[] = "Housekeeping";

extern boolean hasBattery;

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
    ESP_LOGI(TAG, "WifiLoop %u bytes left | Taskstate = %d | core = %u",
      uxTaskGetStackHighWaterMark(WifiManager::wifiManagerTask), eTaskGetState(WifiManager::wifiManagerTask), xTaskGetAffinity(WifiManager::wifiManagerTask));
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
    if (hasBattery) {
      Battery::readVoltage();
      switch (Power::getRunMode()) {
        case RM_FULL:
          if (!Battery::usbPowerPresent() && Battery::getBatteryLevelInPercent(Battery::getBatteryLevelInmV()) < 50) {
            ESP_LOGI(TAG, "Switching to Battery power!");
            // TODO: check against menu driven sleep action re callouts to scd40 and deepsleep
            Power::setRunMode(RM_LOW);
          }
          break;
        case RM_LOW:
          /*          if (Battery::usbPowerPresent()) {
                      ESP_LOGI(TAG, "Switching to USB power!");
                      Power::setRunMode(RM_FULL);
                      // @TODO: might need reboot to properly initialise everything
                    }*/
          break;
        default:
          break;
      }
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