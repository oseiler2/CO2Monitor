#include <logging.h>
#include <nvs_config.h>
#include <nvs_flash.h>
#include <nvs.h>

// Local logging tag
static const char TAG[] = __FILE__;

//#include <Preferences.h>

namespace NVS {

  static const char* nvsNamespace = "co2mon";

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

  void close() {
    if (!initialised) return;
    nvs_close(handle);
  }

}