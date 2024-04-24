#include <logging.h>
#include <sd_card.h>
#include <config.h>

#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>
#include <esp_vfs_fat.h>
#include <sys/stat.h>

#include <model.h>
#include <configManager.h>
#include <fileDataLogger.h>

// Local logging tag
static const char TAG[] = __FILE__;

namespace SdCard {

  static boolean initialised = false;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
  };

  sdmmc_card_t* card;

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  sdmmc_slot_config_t slot_config;

  const char* mount_point = SD_MOUNT_POINT;

  boolean probe() {
#if CONFIG_IDF_TARGET_ESP32S3
    slot_config.clk = (gpio_num_t)config.sdClk;
    slot_config.cmd = (gpio_num_t)config.sdCmd;
    slot_config.d0 = (gpio_num_t)config.sdDat0;
    slot_config.d1 = (gpio_num_t)config.sdDat1;
    slot_config.d2 = (gpio_num_t)config.sdDat2;
    slot_config.d3 = (gpio_num_t)config.sdDat3;
    slot_config.d4 = GPIO_NUM_NC;
    slot_config.d5 = GPIO_NUM_NC;
    slot_config.d6 = GPIO_NUM_NC;
    slot_config.d7 = GPIO_NUM_NC;
    slot_config.cd = (gpio_num_t)config.sdDetect;
    slot_config.wp = SDMMC_SLOT_NO_WP;
    slot_config.width = 4;
    slot_config.flags = 0;

    boolean cardDetected = !digitalRead(config.sdDetect);
#elif CONFIG_IDF_TARGET_ESP32
    boolean cardDetected = false;
#endif
    //    ESP_LOGD(TAG, "Card detected ? %u", cardDetected);
    return cardDetected;
  }

  boolean setup() {
    if (!probe()) return false;
    esp_err_t err;
    //    ESP_LOGD(TAG, "Mounting SD card FS");
    err = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (err != ESP_OK) {
      if (err == ESP_FAIL) {
        ESP_LOGE(TAG, "Failed to mount filesystem.");
      } else {
        ESP_LOGE(TAG, "Failed to initialize the card (%s). ", esp_err_to_name(err));
      }
      return false;
    }
    //ESP_LOGD(TAG, "SD card Filesystem mounted");

#ifdef SHOW_DEBUG_MSGS
//    sdmmc_card_print_info(stdout, card);
#endif
    initialised = true;
    return true;
  }

  boolean isInitialised() {
    return initialised;
  }

  boolean writeEvent(int16_t mask, Model* model, TrafficLightStatus status) {
    if (!initialised) {
      ESP_LOGE(TAG, "SC card not initiased!");
      return false;
    }
    return FileDataLogger::writeEvent(mount_point, mask, model, status);
  }

  boolean unmount() {
    initialised = false;
    esp_err_t err = esp_vfs_fat_sdcard_unmount(mount_point, card);
    if (err != ESP_OK) {
      ESP_LOGI(TAG, "Error unmounting card (%s)", esp_err_to_name(err));
      return false;
    }
    //    ESP_LOGD(TAG, "Card unmounted");
    return true;
  }

}