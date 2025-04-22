#include <logging.h>
#include <nvs_config.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <power.h>

// Local logging tag
static const char TAG[] = "NVSConfig";

//#include <Preferences.h>

namespace NVS {

  static const char* nvsNamespace = "co2mon";
  static const char* KEY_RUNMODE = "runmode";

  static boolean initialised = false;
  static nvs_handle_t handle;

  boolean init() {
    if (initialised) return true;
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Error initialising nvs (%s)", esp_err_to_name(err));
      return false;
    }
    /*
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // NVS partition was truncated and needs to be erased
            // Retry nvs_flash_init
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }*/

    err = nvs_open(nvsNamespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
      ESP_LOGI(TAG, "Error opening nvs handle (%s)", esp_err_to_name(err));
      return false;
    }
    /*
        nvs_stats_t nvs_stats;
        err = nvs_get_stats(NULL, &nvs_stats);
        if (err != ESP_OK) {
          ESP_LOGD(TAG, "Error getting nvs stats (%s)", esp_err_to_name(err));
        } else {
          ESP_LOGI(TAG, "Count: UsedEntries = (%lu), FreeEntries = (%lu), TotalEntries = (%lu), namespace count = (%lu)",
            nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries, nvs_stats.namespace_count);
        }

        nvs_iterator_t iter = nvs_entry_find("nvs", nvsNamespace, NVS_TYPE_ANY);
        nvs_entry_info_t info;
        while (iter != NULL) {
          nvs_entry_info(iter, &info);
          iter = nvs_entry_next(iter);
          ESP_LOGI(TAG, "'%s'->'%s' (%d)", info.namespace_name, info.key, info.type);
        }
        nvs_release_iterator(iter);*/

    initialised = true;
    return true;
  }

  boolean isInitialised() {
    return initialised;
  }

  RunMode readRunmode() {
    if (!initialised) {
      init();
    }
    uint8_t runmode = RM_UNDEFINED;
    esp_err_t err = nvs_get_u8(handle, KEY_RUNMODE, &runmode);
    if (err != ESP_OK) {
      ESP_LOGI(TAG, "Error getting RunMode from NVS (%s)", esp_err_to_name(err));
    }
    ESP_LOGD(TAG, "Reading RunMode %u", runmode);
    return (RunMode)runmode;
  }

  boolean writeRunmode(RunMode rm) {
    if (!initialised) {
      init();
    }
    esp_err_t err = nvs_set_u8(handle, KEY_RUNMODE, (uint8_t)rm);
    if (err != ESP_OK) {
      ESP_LOGI(TAG, "Error writing RunMode to NVS (%s)", esp_err_to_name(err));
      return false;
    }
    err = nvs_commit(handle);
    if (err != ESP_OK) {
      ESP_LOGI(TAG, "Error commiting RunMode changes NVS (%s)", esp_err_to_name(err));
      return false;
    }
    ESP_LOGD(TAG, "Successfully written RunMode %u", rm);
    return true;
  }

  void close() {
    if (!initialised) return;
    nvs_close(handle);
  }

}