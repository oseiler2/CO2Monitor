#include <logging.h>
#include <vector>

namespace logging {

  uint8_t recursionGuard = 0;

  std::vector<logCallback_t> callbacks;

  void addOnLogCallback(logCallback_t logCallback) {
    callbacks.push_back(logCallback);
  }

  int logger(const char* format, va_list args) {
    const uint16_t LOG_BUFFER_LEN = 255;
    if (recursionGuard != 0) return 0;
    recursionGuard++;
    static char buffer[LOG_BUFFER_LEN] = { 0 };
    char* finalBuffer = buffer;
    int len = vsnprintf(buffer, LOG_BUFFER_LEN, format, args);
    if (len >= LOG_BUFFER_LEN) {
      finalBuffer = (char*)malloc(len + 1);
      if (finalBuffer == NULL) {
        recursionGuard--;
        return 0;
      }
      len = vsnprintf(finalBuffer, len, format, args);
    }

    // Serial log
    ets_printf("%s\n", finalBuffer);

    // any additional log callbacks
    for (logCallback_t logCallback : callbacks) {
      logCallback(0, "", finalBuffer);
    }

    if (len >= LOG_BUFFER_LEN) {
      free(finalBuffer);
    }
    recursionGuard--;
    return len;
  }

  void decorateLog(esp_log_level_t level, const char* file, int line, const char* function, const char* tag, const char* format, ...) {
    level = min(level, ESP_LOG_VERBOSE);
    level = max(level, ESP_LOG_NONE);

    const char* colour = LOG_LEVEL_COLOURS[level];
    const char* letter = LOG_LEVEL_LETTERS[level];

    const uint16_t LOG_BUFFER_LEN = 200;
    static char buffer[LOG_BUFFER_LEN] = { 0 };
    char* finalBuffer = buffer;

#ifdef COLOUR_CODE_LOG
    int len = snprintf(buffer, LOG_BUFFER_LEN, "%s[%6u][%s][%s:%i] %s(): [%s] %s%s", colour, (unsigned long)(esp_timer_get_time() / 1000ULL), letter, file, line, function, tag, format, ESP_LOG_RESET_COLOUR);
#else
    int len = snprintf(buffer, LOG_BUFFER_LEN, "[%6u][%s][%s:%i] %s(): [%s] %s", (unsigned long)(esp_timer_get_time() / 1000ULL), letter, file, line, function, tag, format);
#endif

    if (len >= LOG_BUFFER_LEN) {
      finalBuffer = (char*)malloc(len + 1);
      if (finalBuffer == NULL) {
        return;
      }
#ifdef COLOUR_CODE_LOG
      len = snprintf(finalBuffer, len, "%s[%6u][%s][%s:%i] %s(): [%s] %s%s", colour, (unsigned long)(esp_timer_get_time() / 1000ULL), letter, file, line, function, tag, format, ESP_LOG_RESET_COLOUR);
#else
      len = snprintf(finalBuffer, len, "[%6u][%s][%s:%i] %s(): [%s] %s", (unsigned long)(esp_timer_get_time() / 1000ULL), letter, file, line, function, tag, format);
#endif
    }
    va_list args;
    va_start(args, format);
    esp_log_writev(level, tag, finalBuffer, args);
    va_end(args);
    if (len >= LOG_BUFFER_LEN) {
      free(finalBuffer);
    }
  }
}