#include <sensors.h>

// Local logging tag
static const char TAG[] = "Sensors";

const uint32_t X_CMD_SCD30_DATA_READY = bit(1);
const uint32_t X_CMD_SHUTDOWN = bit(2);

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

  volatile boolean loopActive = false;

  static void IRAM_ATTR measurementReady() {
    BaseType_t high_task_awoken = pdFALSE;
    if (sensorsTask)
      xTaskNotifyFromISR(sensorsTask, X_CMD_SCD30_DATA_READY, eSetBits, &high_task_awoken);
  }

  void setupSensorsLoop(SCD30* pScd30, SCD40* pScd40, SPS_30* pSps30, BME680* pBme680) {
    scd30 = pScd30;
    scd40 = pScd40;
    sps30 = pSps30;
    bme680 = pBme680;
  }

  TaskHandle_t start(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core) {
    _ASSERT(scd30 || scd40 || sps30 || bme680);
    loopActive = true;
    xTaskCreatePinnedToCore(
      sensorsLoop,  // task function
      name,         // name of task
      stackSize,    // stack size of task
      (void*)1,     // parameter of the task
      priority,     // priority of the task
      &sensorsTask, // task handle
      core);        // CPU core
    if (scd40) lastScd40Reading = millis() - (uint32_t)(scd40->getInterval() * 1000);
    if (sps30) lastSps30Reading = millis() - (uint32_t)(sps30->getInterval() * 1000);
    if (bme680) lastBme680Reading = millis() - (uint32_t)(bme680->getInterval() * 1000);
    if (scd30) {
      lastScd30Reading = millis() - (uint32_t)(scd30->getInterval() * 1000);
      pinMode(SCD30_RDY_PIN, INPUT);
      attachInterrupt(SCD30_RDY_PIN, measurementReady, RISING);
    }
    return sensorsTask;
  }

  void runOnce() {
    if (scd40) scd40->readScd40();
    if (scd30) scd30->readScd30();
    if (sps30) sps30->readSps30();
    if (bme680) bme680->readBme680();
  }

  void shutDownSensorsLoop() {
    ESP_LOGD(TAG, "shutDownSensorsLoop");
    if (loopActive && sensorsTask) {
      xTaskNotify(sensorsTask, X_CMD_SHUTDOWN, eSetBits);
      while (loopActive) {
        vTaskDelay(pdMS_TO_TICKS(50));
      }
      sensorsTask = NULL;
    }
    ESP_LOGD(TAG, "done");
  }

  void sensorsLoop(void* pvParameters) {
    _ASSERT((uint32_t)pvParameters == 1);
    uint32_t taskNotification;
    BaseType_t notified;
    uint32_t now;
    // try a read straight away to see if data is ready
    if (scd40) scd40->readScd40();
    if (scd30) scd30->readScd30();
    if (sps30) sps30->readSps30();
    if (bme680) bme680->readBme680();
    while (loopActive) {
      if (scd40 && (millis() - lastScd40Reading > (uint32_t)(scd40->getInterval() * 1000))) {
        lastScd40Reading += (uint32_t)(scd40->getInterval() * 1000);
        scd40->readScd40();
      }
      if (sps30 && (millis() - lastSps30Reading > (uint32_t)(sps30->getInterval() * 1000))) {
        lastSps30Reading += (uint32_t)(sps30->getInterval() * 1000);
        sps30->readSps30();
      }
      if (bme680 && (millis() - lastBme680Reading > (uint32_t)(bme680->getInterval() * 1000))) {
        lastBme680Reading += (uint32_t)(bme680->getInterval() * 1000);
        bme680->readBme680();
      }

      now = millis();
      uint32_t delay = 0xffffffff;
      if (scd40) {
        uint32_t nextScd40 = (uint32_t)(scd40->getInterval() * 1000) - (now - lastScd40Reading);
        if (nextScd40 > (uint32_t)(scd40->getInterval() * 1000)) nextScd40 = 0;
        delay = min(delay, nextScd40);
      }
      if (scd30) {
        uint32_t nextScd30 = (uint32_t)(scd30->getInterval() * 1000) - (now - lastScd30Reading);
        if (nextScd30 > (uint32_t)(scd30->getInterval() * 1000)) nextScd30 = 0;
        delay = min(delay, nextScd30);
      }
      if (sps30) {
        uint32_t nextSps30 = (uint32_t)(sps30->getInterval() * 1000) - (now - lastSps30Reading);
        if (nextSps30 > (uint32_t)(sps30->getInterval() * 1000)) nextSps30 = 0;
        delay = min(delay, nextSps30);
      }
      if (bme680) {
        uint32_t nextBme680 = (uint32_t)(bme680->getInterval() * 1000) - (now - lastBme680Reading);
        if (nextBme680 > (uint32_t)(bme680->getInterval() * 1000)) nextBme680 = 0;
        delay = min(delay, nextBme680);
      }

      notified = xTaskNotifyWait(0x00,  // Don't clear any bits on entry
        ULONG_MAX,                      // Clear all bits on exit
        &taskNotification,              // Receives the notification value
        pdMS_TO_TICKS(delay));
      if (notified == pdPASS) {
        if (scd30 && taskNotification & X_CMD_SCD30_DATA_READY) {
          taskNotification &= ~X_CMD_SCD30_DATA_READY;
          scd30->readScd30();
          lastScd30Reading += (uint32_t)(scd30->getInterval() * 1000);
        }
        if (taskNotification & X_CMD_SHUTDOWN) {
          taskNotification &= ~X_CMD_SHUTDOWN;
          loopActive = false;
        }
      } else {
        if (scd30 && digitalRead(SCD30_RDY_PIN)) {
          scd30->readScd30();
            lastScd30Reading += (uint32_t)(scd30->getInterval() * 1000);
        }
      }
    }
    vTaskDelete(NULL);
  }
}
