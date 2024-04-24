#include <fileDataLogger.h>
#include <battery.h>

#include <FS.h>
#include <LittleFS.h>

// Local logging tag
static const char TAG[] = __FILE__;

namespace FileDataLogger {

  boolean writeEvent(const char* mountPoint, int16_t mask, Model* model, TrafficLightStatus status, uint16_t batInMV) {
    time_t now;
    struct tm timeinfo;
    FILE* f;
    time(&now);
    localtime_r(&now, &timeinfo);

    char buf[30];
    strftime(buf, 11, "%Y-%m-%d", &timeinfo);

    char fileName[40];
    sprintf(fileName, "%s/data", mountPoint);
    mkdir(fileName, 0777);

    sprintf(fileName, "%s/data/%s.log.csv", mountPoint, buf);
    //    ESP_LOGD(TAG, "Opening file %s", fileName);

    struct stat st;
    if (stat(fileName, &st) == 0) {
      // file exists
//      ESP_LOGD(TAG, "File exists - opening");
      f = fopen(fileName, FILE_APPEND);
    } else {
      // file doesn't exist yet
  //    ESP_LOGD(TAG, "File doesn't exist - creating");
      f = fopen(fileName, FILE_WRITE);
      // write headers
      fprintf(f, "time(DD/MM/YYYY HH:MM:SS),co2(ppm),temperature(°C),humidity(%%rH),iaq,pressure(hPa),trafficlight,battery(%%),battery(mV)\n");
    }
    if (f == NULL) {
      ESP_LOGE(TAG, "Failed to open file %s for writing", fileName);
      return false;
    }

//    strftime(buf, 30, "%Y-%m-%dT%H:%M:%S%z", &timeinfo);
    strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &timeinfo);
    fprintf(f, "%s,", buf);
    if (mask & M_CO2)
      fprintf(f, "\"%u\",", model->getCo2());
    else
      fprintf(f, "\"\",");
    if (mask & M_TEMPERATURE)
      fprintf(f, "\"%.1f\",", model->getTemperature());
    else
      fprintf(f, "\"\",");
    if (mask & M_HUMIDITY)
      fprintf(f, "\"%.1f\",", model->getHumidity());
    else
      fprintf(f, ",");
    if (mask & M_IAQ)
      fprintf(f, "\"%u\",", model->getIAQ());
    else
      fprintf(f, "\"\",");
    if (mask & M_PRESSURE)
      fprintf(f, "\"%u\",", model->getPressure());
    else
      fprintf(f, ",");
    if (status != OFF)
      fprintf(f, "%u,", status);
    else
      fprintf(f, ",");
    if (batInMV > 1000)
      fprintf(f, "%u,", Battery::getBatteryLevelInPercent(batInMV));
    else
      fprintf(f, ",");
    if (batInMV > 1000)
      fprintf(f, "%u,", batInMV);
    else
      fprintf(f, ",");
    fprintf(f, "\n");
    fclose(f);
    //    ESP_LOGD(TAG, "File closed");
    return true;
  }
}
