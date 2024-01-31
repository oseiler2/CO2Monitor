#include <logging.h>
#include <sd_card.h>

// Local logging tag
static const char TAG[] = __FILE__;

namespace SdCard {

  static boolean initialised = false;

  const char mount_point[] = MOUNT_POINT;

  boolean probe() {
    return false;
  }

  boolean setup() {
    if (!probe()) return false;
    return false;
  }

  boolean isInitialised() {
    return initialised;
  }

  boolean writeEvent(int16_t mask, Model* model, TrafficLightStatus status, uint16_t batInMV) {
    if (!initialised) {
      ESP_LOGE(TAG, "SC card not initiased!");
      return false;
    }
    return false;
 }

  boolean unmount() {
    initialised = false;
    return true;
  }

}