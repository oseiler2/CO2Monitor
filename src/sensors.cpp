#include <sensors.h>
#include <Arduino.h>
#include <scd30.h>
#include <scd40.h>
#include <sps_30.h>
#include <bme680.h>

// Local logging tag
static const char TAG[] = __FILE__;

const uint32_t X_CMD_DATA_READY = bit(1);

namespace Sensors {

  TaskHandle_t sensorsTask;

  SCD30* scd30;
  SCD40* scd40;
  SPS_30* sps30;
  BME680* bme680;

  uint32_t lastScd30Reading = 0;
  uint32_t lastScd40Reading = 0;
  uint32_t lastSps30Reading = 0;
  uint32_t lastBme680Reading = 0;

  static void IRAM_ATTR measurementReady() {
    BaseType_t high_task_awoken = pdFALSE;
    if (sensorsTask)
      xTaskNotifyFromISR(sensorsTask, X_CMD_DATA_READY, eSetBits, &high_task_awoken);
  }

  void setupSensorsLoop(SCD30* pScd30, SCD40* pScd40, SPS_30* pSps30, BME680* pBme680) {
    scd30 = pScd30;
    scd40 = pScd40;
    sps30 = pSps30;
    bme680 = pBme680;
  }

  TaskHandle_t start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core) {
    xTaskCreatePinnedToCore(
      sensorsLoop,  // task function
      name,         // name of task
      stackSize,    // stack size of task
      (void*)1,     // parameter of the task
      priority,     // priority of the task
      &sensorsTask, // task handle
      core);        // CPU core
    lastScd30Reading = millis();
    lastScd40Reading = millis();
    lastSps30Reading = millis();
    lastBme680Reading = millis();
    if (scd30) {
      pinMode(SCD30_RDY_PIN, INPUT);
      attachInterrupt(SCD30_RDY_PIN, measurementReady, RISING);
    }
    return sensorsTask;
  }

  void sensorsLoop(void* pvParameters) {
    _ASSERT((uint32_t)pvParameters == 1);
    uint32_t taskNotification;
    BaseType_t notified;
    uint32_t now;
    while (1) {
      if (scd40 && (millis() - lastScd40Reading > (uint32_t)(scd40->getInterval() * 1000))) {
        lastScd40Reading += scd40->getInterval() * 1000;
        scd40->readScd40();
      }
      if (sps30 && (millis() - lastSps30Reading > (uint32_t)(sps30->getInterval() * 1000))) {
        lastSps30Reading += sps30->getInterval() * 1000;
        sps30->readSps30();
      }
      if (bme680 && (millis() - lastBme680Reading > (uint32_t)(bme680->getInterval() * 1000))) {
        lastBme680Reading += bme680->getInterval() * 1000;
        bme680->readBme680();
      }

      now = millis();
      uint32_t delay = 0xffffffff;
      if (scd40) {
        uint32_t nextScd40 = lastScd40Reading + scd40->getInterval() * 1000 - now;
        if (nextScd40 > scd40->getInterval() * 1000) nextScd40 = 0;
        delay = min(delay, nextScd40);
      }
      if (scd30) {
        uint32_t nextScd30 = lastScd30Reading + scd30->getInterval() * 1000 - now;
        if (nextScd30 > scd30->getInterval() * 1000) nextScd30 = 0;
        delay = min(delay, nextScd30);
      }
      if (sps30) {
        uint32_t nextSps30 = lastSps30Reading + sps30->getInterval() * 1000 - now;
        if (nextSps30 > sps30->getInterval() * 1000) nextSps30 = 0;
        delay = min(delay, nextSps30);
      }
      if (bme680) {
        uint32_t nextBme680 = lastBme680Reading + bme680->getInterval() * 1000 - now;
        if (nextBme680 > bme680->getInterval() * 1000) nextBme680 = 0;
        delay = min(delay, nextBme680);
      }

      if (scd30) {
        if (delay > 10)
          notified = xTaskNotifyWait(0x00,  // Don't clear any bits on entry
            ULONG_MAX,                      // Clear all bits on exit
            &taskNotification,              // Receives the notification value
            pdMS_TO_TICKS(delay));
        if (delay > 10 && notified == pdPASS) {
          if (taskNotification & X_CMD_DATA_READY) {
            taskNotification &= ~X_CMD_DATA_READY;
            scd30->readScd30();
            lastScd30Reading += scd30->getInterval() * 1000;
          }
        } else {
          if (digitalRead(SCD30_RDY_PIN)) {
            scd30->readScd30();
            lastScd30Reading += scd30->getInterval() * 1000;
          }
        }
      } else {
        if (delay > 10) {
          vTaskDelay(pdMS_TO_TICKS(delay));
        }
      }
    }
    vTaskDelete(NULL);
  }
}
