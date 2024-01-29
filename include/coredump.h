#include <logging.h>
#include <globals.h>
#include <Arduino.h>
#include <config.h>
#include <stdio.h>
#include <esp_core_dump.h>
#include <esp_partition.h>

namespace coredump {
  boolean init();
  boolean checkForCoreDump();
  void logCoreDumpSummary();
  boolean writeCoreDumpToFile();

}