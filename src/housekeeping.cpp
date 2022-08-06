#include <housekeeping.h>
#include <mqtt.h>
#include <i2c.h>
#include <ota.h>
#include <sensors.h>
#include <power.h>
#include <battery.h>

// Local logging tag
static const char TAG[] = __FILE__;

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
    if (Sensors::sensorsTask) {
      ESP_LOGD(TAG, "SensorsLoop %d bytes left | Taskstate = %d",
        uxTaskGetStackHighWaterMark(Sensors::sensorsTask), eTaskGetState(Sensors::sensorsTask));
    }

    if (ESP.getMinFreeHeap() <= 2048) {
      ESP_LOGW(TAG,
        "Memory full, counter cleared (heap low water mark = %d Bytes / "
        "free heap = %d bytes)",
        ESP.getMinFreeHeap(), ESP.getFreeHeap());
      Serial.flush();
      esp_restart();
    }

    Battery::readVoltage();
    switch (Power::getPowerMode()) {
      case USB:
        if (!Battery::usbPowerPresent() && Battery::getBatteryLevelInPercent(Battery::getBatteryLevelInmV()) < 50) {
          ESP_LOGI(TAG, "Switching to Battery power!");
          Power::setPowerMode(BATTERY);
        }
        break;
      case BATTERY:
        if (Battery::usbPowerPresent()) {
          ESP_LOGI(TAG, "Switching to USB power!");
          Power::setPowerMode(USB);
          // @TODO: might need reboot to properly initialise everything
        }
        break;
      default:
        break;
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