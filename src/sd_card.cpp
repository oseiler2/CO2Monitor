#include <sd_card.h>
#include <config.h>
#include <power.h>

#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"

#include <model.h>


// Local logging tag
static const char TAG[] = __FILE__;

FILE* f;

namespace SdCard {

#define MOUNT_POINT "/sdcard"

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
  };

  sdmmc_card_t* card;

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  sdmmc_slot_config_t slot_config = {
      .clk = (gpio_num_t)SD_CLK,
      .cmd = (gpio_num_t)SD_CMD,
      .d0 = (gpio_num_t)SD_DAT0,
      .d1 = (gpio_num_t)SD_DAT1,
      .d2 = (gpio_num_t)SD_DAT2,
      .d3 = (gpio_num_t)SD_DAT3,
      .d4 = GPIO_NUM_NC,
      .d5 = GPIO_NUM_NC,
      .d6 = GPIO_NUM_NC,
      .d7 = GPIO_NUM_NC,
      .cd = (gpio_num_t)SD_DETECT,
      .wp = SDMMC_SLOT_NO_WP,
      .width = 4,
      .flags = 0,
  };

  const char mount_point[] = MOUNT_POINT;

  boolean probe() {
    pinMode(SD_DETECT, INPUT);
    boolean cardDetected = !digitalRead(SD_DETECT);
    //    ESP_LOGD(TAG, "Card detected ? %u", cardDetected);
    return cardDetected;
  }

  boolean setup() {
    if (!probe()) return false;
    esp_err_t ret;
    //    ESP_LOGD(TAG, "Mounting SD card FS");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
        ESP_LOGE(TAG, "Failed to mount filesystem.");
      } else {
        ESP_LOGE(TAG, "Failed to initialize the card (%s). ", esp_err_to_name(ret));
      }
      return false;
    }
    //ESP_LOGD(TAG, "SD card Filesystem mounted");

#ifdef SHOW_DEBUG_MSGS
//    sdmmc_card_print_info(stdout, card);
#endif
    return true;
  }

  boolean writeEvent(int16_t mask, Model* model, TrafficLightStatus status, uint16_t batInMV) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    char buf[30];
    strftime(buf, 11, "%Y-%m-%d", &timeinfo);

    char fileName[30];

    sprintf(fileName, "%s/%s.log.csv", MOUNT_POINT, buf);
    //    ESP_LOGD(TAG, "filename: %s", fileName);
    ESP_LOGD(TAG, "Opening file %s", fileName);

    struct stat st;
    if (stat(fileName, &st) == 0) {
      // file exists
//      ESP_LOGD(TAG, "File exists - opening");
      f = fopen(fileName, "a");
    } else {
      // file doesn't exist yet
  //    ESP_LOGD(TAG, "File doesn't exist - creating");
      f = fopen(fileName, "w");
      // write headers
      fprintf(f, "time,co2,temperature,humidity,iaq,pressure,trafficlight,bat\n");
    }
    if (f == NULL) {
      ESP_LOGE(TAG, "Failed to open file %s for writing", fileName);
      return false;
    }

    strftime(buf, 30, "%Y-%m-%dT%H:%M:%S%z", &timeinfo);
    fprintf(f, "%s,", buf);
    if (mask & M_CO2)
      fprintf(f, "%u,", model->getCo2());
    else
      fprintf(f, ",");
    if (mask & M_TEMPERATURE)
      fprintf(f, "%.1f,", model->getTemperature());
    else
      fprintf(f, ",");
    if (mask & M_HUMIDITY)
      fprintf(f, "%.1f,", model->getHumidity());
    else
      fprintf(f, ",");
    if (mask & M_IAQ)
      fprintf(f, "%u,", model->getIAQ());
    else
      fprintf(f, ",");
    if (mask & M_PRESSURE)
      fprintf(f, "%u,", model->getPressure());
    else
      fprintf(f, ",");
    if (status != UNDEFINED)
      fprintf(f, "%u,", status);
    else
      fprintf(f, ",");
    if (batInMV > 1000)
      fprintf(f, "%u,", batInMV);
    else
      fprintf(f, ",");
    fprintf(f, "\n");
    fclose(f);
    f = NULL;
    //    ESP_LOGD(TAG, "File closed");
    return true;
  }

  boolean unmount() {
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(mount_point, card);
    if (ret != ESP_OK) {
      ESP_LOGI(TAG, "Error unmounting card (%s)", esp_err_to_name(ret));
      return false;
    }
    //    ESP_LOGD(TAG, "Card unmounted");
    return true;
  }

}