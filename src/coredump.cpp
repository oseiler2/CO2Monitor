#include <coredump.h>
#include <sd_card.h>
#include <logging.h>
#include <sys/stat.h>

// Local logging tag
static const char TAG[] = __FILE__;

namespace coredump {

#define PARTITION_NAME "coredump"

  boolean init() {
    esp_core_dump_init();
    return true;
  }

  boolean checkForCoredump() {
    esp_err_t err = esp_core_dump_image_check();
    if (err == ESP_ERR_NOT_FOUND) return false;
    if (err != ESP_OK) {
      ESP_LOGD(TAG, "Error checking coredump (%s)", esp_err_to_name(err));
    }
    return (err == ESP_OK);
  }

  void logCoredumpSummary() {
    if (!checkForCoredump()) return;
    esp_core_dump_summary_t* summary = (esp_core_dump_summary_t*)malloc(sizeof(esp_core_dump_summary_t));
    esp_err_t err = esp_core_dump_get_summary(summary);
    if (err != ESP_OK) {
      ESP_LOGD(TAG, "Error checking coredump (%s)", esp_err_to_name(err));
    }
    if (err == ESP_OK) {
      ESP_LOGD(TAG, "Coredump summary: ");
      char* buf = (char*)malloc(16 + 1);
      strncpy(buf, summary->exc_task, 16);
      buf[16] = 0x00;
      ESP_LOGI(TAG, "exc_task: %s", buf);
    }
    free(summary);
  }

  boolean getCoredumpSize(size_t* out_addr, size_t* out_size) {
    if (!checkForCoredump()) return false;
    esp_err_t err = esp_core_dump_image_get(out_addr, out_size);
    if (err != ESP_OK) {
      ESP_LOGD(TAG, "Error getting coredump image (%s)", esp_err_to_name(err));
      return false;
    }
    ESP_LOGI(TAG, "Coredump addr: %d", *out_addr);
    ESP_LOGI(TAG, "Coredump size: %d", *out_size);
    if (*out_size <= 0) {
      ESP_LOGE(TAG, "Size of coredump <= 0");
      return false;
    }
    return true;
  }

  boolean getCoreDump(size_t* out_addr, size_t* out_size, esp_partition_t** partition) {
    getCoredumpSize(out_addr, out_size);

    esp_partition_type_t p_type = ESP_PARTITION_TYPE_DATA;
    esp_partition_subtype_t p_subtype = ESP_PARTITION_SUBTYPE_DATA_COREDUMP;
    const char* label = PARTITION_NAME;
    *partition = (esp_partition_t*)esp_partition_find_first(p_type, p_subtype, label);

    if (partition == NULL) {
      ESP_LOGE(TAG, "Coredump partition not found");
      return false;
    }
    return true;
  }

  boolean writeCoreDumpToFileInt(FILE* f) {
    esp_err_t err;

    size_t out_addr;
    size_t out_size;
    esp_partition_t* partition;
    if (!getCoreDump(&out_addr, &out_size, &partition)) return false;

    const uint16_t BUFFER_SIZE = 256;
    static uint8_t buffer[BUFFER_SIZE];
    uint16_t fullPages = (out_size / BUFFER_SIZE);
    ESP_LOGD(TAG, "fullPages: %d", fullPages);
    for (uint16_t i = 0; i < fullPages; i++) {
      err = esp_partition_read(partition, i * BUFFER_SIZE, buffer, BUFFER_SIZE);
      if (err != ESP_OK) {
        ESP_LOGD(TAG, "Error reading from partition (%s)", esp_err_to_name(err));
        return false;
      }
      if (fwrite(buffer, 1, BUFFER_SIZE, f) != BUFFER_SIZE) {
        ESP_LOGD(TAG, "Error writing to file (%s)", esp_err_to_name(err));
        return false;
      }
      taskYIELD();
    }
    uint16_t remainder = out_size - fullPages * BUFFER_SIZE;
    if (remainder > 0) {
      ESP_LOGD(TAG, "remainder: %d", remainder);
      err = esp_partition_read(partition, fullPages * BUFFER_SIZE, buffer, remainder);
      if (err != ESP_OK) {
        ESP_LOGD(TAG, "Error reading from partition (%s)", esp_err_to_name(err));
        return false;
      }
      if (fwrite(buffer, 1, remainder, f) != remainder) {
        ESP_LOGD(TAG, "Error writing to file (%s)", esp_err_to_name(err));
        return false;
      }
      taskYIELD();
    }
    return true;
  }

  boolean writeCoredumpToFile() {
    if (!checkForCoredump()) return false;
    if (!SdCard::isInitialised()) {
      ESP_LOGE(TAG, "SD card not initiased!");
      return false;
    }
    time_t now;
    struct tm timeinfo;
    FILE* f;

    time(&now);
    localtime_r(&now, &timeinfo);

    char buf[30];
    strftime(buf, 20, "%Y-%m-%d_%H-%M-%S", &timeinfo);

    char fileName[41]; // 7 + 10 + 19 + 4

    sprintf(fileName, "%s/coredump-%s.bin", SD_MOUNT_POINT, buf);
    ESP_LOGD(TAG, "Opening file %s", fileName);

    struct stat st;
    if (stat(fileName, &st) == 0) {
      // file exists
      ESP_LOGD(TAG, "File already exists - abort");
      return false;
    } else {
      // file doesn't exist yet
      ESP_LOGD(TAG, "File doesn't exist - creating");
      f = fopen(fileName, "w");
    }
    if (f == NULL) {
      ESP_LOGE(TAG, "Failed to open file %s for writing", fileName);
      return false;
    }
    boolean res = writeCoreDumpToFileInt(f);

    fclose(f);
    ESP_LOGD(TAG, "File closed");
    if (res) {
      ESP_LOGD(TAG, "Coredump successfully written to file %s", fileName);
      eraseCoredump();
    }
    return res;
  }

  boolean eraseCoredump() {
    //    if (!checkForCoredump()) return false;
    esp_err_t err = esp_core_dump_image_erase();
    if (err == ESP_OK) {
      ESP_LOGD(TAG, "Coredump image erased successfully");
    } else {
      ESP_LOGE(TAG, "Error erasing coredump image (%s)", esp_err_to_name(err));
    }
    return (err == ESP_OK);
  }


  boolean writeCoredumpToStream(Print* p) {
    if (!checkForCoredump()) return false;
    esp_err_t err;

    size_t out_addr;
    size_t out_size;
    esp_partition_t* partition;
    if (!getCoreDump(&out_addr, &out_size, &partition)) return false;

    const uint16_t BUFFER_SIZE = 256;
    static uint8_t buffer[BUFFER_SIZE];
    uint16_t fullPages = (out_size / BUFFER_SIZE);
    ESP_LOGD(TAG, "fullPages: %d", fullPages);
    size_t written;
    for (uint16_t i = 0; i < fullPages; i++) {
      err = esp_partition_read(partition, i * BUFFER_SIZE, buffer, BUFFER_SIZE);
      if (err != ESP_OK) {
        ESP_LOGD(TAG, "Error reading from partition (%s)", esp_err_to_name(err));
        return false;
      }
      written = p->write(buffer, BUFFER_SIZE);
      taskYIELD();
      if (written != BUFFER_SIZE) {
        ESP_LOGD(TAG, "Error writing to stream, written: %d, expected: %d", written, BUFFER_SIZE);
        return false;
      }
    }
    uint16_t remainder = out_size - fullPages * BUFFER_SIZE;
    if (remainder > 0) {
      ESP_LOGD(TAG, "remainder: %d", remainder);
      err = esp_partition_read(partition, fullPages * BUFFER_SIZE, buffer, remainder);
      if (err != ESP_OK) {
        ESP_LOGD(TAG, "Error reading from partition (%s)", esp_err_to_name(err));
        return false;
      }
      written = p->write(buffer, remainder);
      taskYIELD();
      if (written != remainder) {
        ESP_LOGD(TAG, "Error writing to stream, written: %d, expected: %d", written, remainder);
        return false;
      }
    }
    return true;
  }
}