#include <ota.h>
#include <ota_cert.h>
#include <esp32fota.h>
#include <Arduino.h>
#include <config.h>
#include <Ticker.h>

namespace OTA {

  Ticker cyclicTimer;
  preUpdateCallback_t preUpdateCallback;

  void setupOta(preUpdateCallback_t _preUpdateCallback) {
    preUpdateCallback = _preUpdateCallback;
#ifdef OTA_POLL
    cyclicTimer.attach(1060 * 60 * 24, checkForUpdate);
#endif
  }

  const uint32_t X_CMD_CHECK_FOR_UPDATE = bit(1);
  TaskHandle_t otaTask;

  void checkForUpdate() {
    xTaskNotify(otaTask, X_CMD_CHECK_FOR_UPDATE, eSetBits);
  }

  void checkForUpdateInternal() {
    WiFiClientSecure clientForOta;
    secureEsp32FOTA secureEsp32FOTA(OTA_APP, APP_VERSION);

    secureEsp32FOTA._host = OTA_HOST;
    secureEsp32FOTA._descriptionOfFirmwareURL = OTA_URL;
    secureEsp32FOTA._certificate = const_cast<char*>(ota_cert);
    secureEsp32FOTA.clientForOta = clientForOta;

    bool shouldExecuteFirmwareUpdate = secureEsp32FOTA.execHTTPSCheck();
    if (shouldExecuteFirmwareUpdate) {
      ESP_LOGD(TAG, "Firmware update available");
      if (preUpdateCallback) preUpdateCallback();
      secureEsp32FOTA.executeOTA();
    } else {
      ESP_LOGD(TAG, "No firmware update available");
    }
    ESP_LOGD(TAG, "OTA done");
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
        }
      }
    }
    vTaskDelete(NULL);
  }

}
