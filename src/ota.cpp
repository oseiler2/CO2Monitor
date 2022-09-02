#include <Arduino.h>
#include <config.h>
#include <mqtt.h>
#include <ota.h>
#include <esp32fota.h>
#include <Ticker.h>
#include <LittleFS.h>

// Local logging tag
static const char TAG[] = __FILE__;

namespace OTA {

  Ticker cyclicTimer;
  preUpdateCallback_t preUpdateCallback;
  String forceUpdateURL;

  void setupOta(preUpdateCallback_t _preUpdateCallback) {
    preUpdateCallback = _preUpdateCallback;
#ifdef OTA_POLL
    cyclicTimer.attach(1060 * 60 * 24, checkForUpdate);
#endif
  }

  const uint32_t X_CMD_CHECK_FOR_UPDATE = bit(1);
  const uint32_t X_CMD_FORCE_UPDATE = bit(2);
  TaskHandle_t otaTask;

  void checkForUpdate() {
    xTaskNotify(otaTask, X_CMD_CHECK_FOR_UPDATE, eSetBits);
  }

  void checkForUpdateInternal() {
    esp32FOTA esp32FOTA(OTA_APP, APP_VERSION, LittleFS, false, false);
    esp32FOTA.checkURL = String(OTA_URL);
    bool shouldExecuteFirmwareUpdate = esp32FOTA.execHTTPcheck();
    if (shouldExecuteFirmwareUpdate) {
      ESP_LOGD(TAG, "Firmware update available");
      if (preUpdateCallback) preUpdateCallback();
      mqtt::sendStatus("Starting OTA update");
      esp32FOTA.execOTA();
    } else {
      ESP_LOGD(TAG, "No firmware update available");
    }
    ESP_LOGD(TAG, "OTA done");
  }

  void forceUpdate(char* url) {
    forceUpdateURL = String(url);
    xTaskNotify(otaTask, X_CMD_FORCE_UPDATE, eSetBits);
  }

  void forceUpdateInternal() {
    ESP_LOGD(TAG, "Beginning forced OTA");
    if (preUpdateCallback) preUpdateCallback();
    esp32FOTA esp32FOTA(OTA_APP, APP_VERSION, LittleFS, false, false);
    mqtt::sendStatus("Starting forced OTA update");
    esp32FOTA.forceUpdate(forceUpdateURL, false);
    forceUpdateURL = "";
    ESP_LOGD(TAG, "Forced OTA done");    forceUpdateURL = "";
  }

  void otaLoop(void* pvParameters) {
    _ASSERT((uint32_t)pvParameters == 1);
    uint32_t taskNotification;
    BaseType_t notified;
    while (1) {
      notified = xTaskNotifyWait(0x00,     // Don't clear any bits on entry
        ULONG_MAX,                         // Clear all bits on exit
        &taskNotification,                 // Receives the notification value
        pdMS_TO_TICKS(100));
      if (notified == pdPASS) {
        if (taskNotification & X_CMD_CHECK_FOR_UPDATE) {
          taskNotification &= ~X_CMD_CHECK_FOR_UPDATE;
          checkForUpdateInternal();
        } else if (taskNotification & X_CMD_FORCE_UPDATE) {
          taskNotification &= ~X_CMD_FORCE_UPDATE;
          forceUpdateInternal();
        }
      }
    }
    vTaskDelete(NULL);
  }

}
