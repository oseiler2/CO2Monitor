#ifndef _LOGGING_H
#define _LOGGING_H

#include <Arduino.h>

//#define USE_ESP_IDF_LOG
namespace logging {
#define ESP_LOG_COLOUR_BLACK "30"
#define ESP_LOG_COLOUR_RED "31"     // ERROR
#define ESP_LOG_COLOUR_GREEN "32"   // INFO
#define ESP_LOG_COLOUR_YELLOW "33"  // WARNING
#define ESP_LOG_COLOUR_BLUE "34"
#define ESP_LOG_COLOUR_MAGENTA "35"
#define ESP_LOG_COLOUR_CYAN "36"     // DEBUG
#define ESP_LOG_COLOUR_GRAY "37"     // VERBOSE
#define ESP_LOG_COLOUR_WHITE "38"
#define ESP_LOG_SECRET_BEGIN "\033[5m"
#define ESP_LOG_SECRET_END "\033[6m"
#define LOG_SECRET(x) ESP_LOG_SECRET_BEGIN x ESP_LOG_SECRET_END

#define ESP_LOG_COLOUR(COLOUR) "\033[0;" COLOUR "m"
#define ESP_LOG_BOLD(COLOUR) "\033[1;" COLOUR "m"
#define ESP_LOG_RESET_COLOUR "\033[0m"

  static const char* const LOG_LEVEL_COLOURS[] = {
      "",                                              // NONE
      ESP_LOG_BOLD(ESP_LOG_COLOUR_RED),        // ERROR
      ESP_LOG_COLOUR(ESP_LOG_COLOUR_YELLOW),   // WARNING
      ESP_LOG_COLOUR(ESP_LOG_COLOUR_GREEN),    // INFO
      ESP_LOG_COLOUR(ESP_LOG_COLOUR_CYAN),     // DEBUG
      ESP_LOG_COLOUR(ESP_LOG_COLOUR_GRAY),     // VERBOSE
  };
  static const char* const LOG_LEVEL_LETTERS[] = {
      "",    // NONE
      "E",   // ERROR
      "W",   // WARNING
      "I",   // INFO
      "D",   // DEBUG
      "V",   // VERBOSE
  };

  typedef void (*logCallback_t)(int, const char*, const char*);

  int logger(const char* format, va_list args);

  void decorateLog(esp_log_level_t level, const char* file, int line, const char* function, const char* tag, const char* format, ...);


  void addOnLogCallback(logCallback_t logCallback);


#define COLOUR_CODE_LOG

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#undef ESP_LOGV

#define ESP_LOGE(tag, format, ...) logging::decorateLog(ESP_LOG_ERROR, pathToFileName(__FILE__), __LINE__, __FUNCTION__, tag, format, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) logging::decorateLog(ESP_LOG_WARN, pathToFileName(__FILE__), __LINE__, __FUNCTION__, tag, format, ##__VA_ARGS__)
#define ESP_LOGI(tag, format, ...) logging::decorateLog(ESP_LOG_INFO, pathToFileName(__FILE__), __LINE__, __FUNCTION__, tag, format, ##__VA_ARGS__)
#define ESP_LOGD(tag, format, ...) logging::decorateLog(ESP_LOG_DEBUG, pathToFileName(__FILE__), __LINE__, __FUNCTION__, tag, format, ##__VA_ARGS__)
#define ESP_LOGV(tag, format, ...) logging::decorateLog(ESP_LOG_VERBOSE, pathToFileName(__FILE__), __LINE__, __FUNCTION__, tag, format, ##__VA_ARGS__)

#undef log_e
#undef log_w
#undef log_i
#undef log_d
#undef log_v

#define log_e(format, ...) logging::decorateLog(ESP_LOG_ERROR, pathToFileName(__FILE__), __LINE__, __FUNCTION__, "", format, ##__VA_ARGS__)
#define log_w(format, ...) logging::decorateLog(ESP_LOG_WARN, pathToFileName(__FILE__), __LINE__, __FUNCTION__, "", format, ##__VA_ARGS__)
#define log_i(format, ...) logging::decorateLog(ESP_LOG_INFO, pathToFileName(__FILE__), __LINE__, __FUNCTION__, "", format, ##__VA_ARGS__)
#define log_d(format, ...) logging::decorateLog(ESP_LOG_DEBUG, pathToFileName(__FILE__), __LINE__, __FUNCTION__, "", format, ##__VA_ARGS__)
#define log_v(format, ...) logging::decorateLog(ESP_LOG_VERBOSE, pathToFileName(__FILE__), __LINE__, __FUNCTION__, "", format, ##__VA_ARGS__)
}

#endif