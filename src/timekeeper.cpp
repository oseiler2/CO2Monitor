#include <config.h>
#include <timekeeper.h>

#include <esp_sntp.h>

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

// Local logging tag
static const char TAG[] = "Timekeeper";

namespace Timekeeper {

  boolean synchronised = false;

  void printTime() {
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in NZ is: %s", strftime_buf);
  }

  void init() {
    setenv("TZ", "NZST-12NZDT,M9.5.0,M4.1.0/3", 1);
    tzset();
  }

  void syncNotificationCallback(timeval* tv) {
    synchronised = true;
    printTime();
  }

  void initSntp() {
    if (sntp_enabled()) return;

    sntp_setoperatingmode(SNTP_OPMODE_POLL);

#if SNTP_MAX_SERVERS >= 1
    sntp_setservername(0, "nz.pool.ntp.org");
#endif
#if SNTP_MAX_SERVERS >= 2
    sntp_setservername(1, "time.cloudflare.com");
#endif
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_set_time_sync_notification_cb(syncNotificationCallback);

    sntp_init();
    ESP_LOGI(TAG, "List of configured NTP servers:");

    for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i) {
      if (sntp_getservername(i)) {
        ESP_LOGI(TAG, "server %d: %s", i, sntp_getservername(i));
      } else {
        // we have either IPv4 or IPv6 address, let's print it
        char buff[INET6_ADDRSTRLEN];
        ip_addr_t const* ip = sntp_getserver(i);
        if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)
          ESP_LOGI(TAG, "server %d: %s", i, buff);
      }
    }
    /*
        // wait for time to be set
        int retry = 0;
        const int retry_count = 15;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
          ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
          vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
printTime();
    */
  }

  boolean isSynchronised() {
    return synchronised;
    //    return (sntp_get_sync_status() != SNTP_SYNC_STATUS_IN_PROGRESS);
  }
}