#include <rtc_osc.h>
#include "soc/rtc.h"

// Local logging tag
static const char TAG[] = "RtcOsc";

#define CALIBRATE_ONE(cali_clk) calibrate_one(cali_clk, #cali_clk)

namespace RtcOsc {

  static uint32_t calibrate_one(rtc_cal_sel_t cal_clk, const char* name) {
    const uint32_t cal_count = 1000;
    const float factor = (1 << 19) * 1000.0f;
    uint32_t cali_val;
    ESP_LOGI(TAG, "%s:", name);
    for (int i = 0; i < 5; ++i) {
      cali_val = rtc_clk_cal(cal_clk, cal_count);
      ESP_LOGD(TAG, "calibrate (%d): %.3f kHz", i, factor / (float)cali_val);
    }
    return cali_val;
  }

  void setupRtc() {
    if (!rtc_clk_32k_enabled()) {
      rtc_clk_32k_bootstrap(512);
      rtc_clk_32k_bootstrap(512);
      rtc_clk_32k_enable(true);
      uint32_t cal_32k = CALIBRATE_ONE(RTC_CAL_32K_XTAL);
      rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
      if (cal_32k == 0) {
        ESP_LOGW(TAG, "32K XTAL OSC has not started up");
      }
    }
    if (rtc_clk_32k_enabled()) {
      //      ESP_LOGI(TAG, "OSC Enabled");
    }
  }

}
