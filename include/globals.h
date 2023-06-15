#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <logging.h>
#include <Arduino.h>

#ifndef _ASSERT
#define _ASSERT(cond)                                                          \
  if ((cond) == 0) {                                                           \
    ESP_LOGE(TAG, "FAILURE in %s:%d", __FILE__, __LINE__);                     \
    /*mask_user_IRQ();*/                                                           \
    for (;;)                                                                   \
      ;                                                                        \
  }
#endif

#endif