#include <logging.h>
#include <globals.h>
#include <Arduino.h>
#include <config.h>
#include <stdio.h>
#include <esp_core_dump.h>
#include <esp_partition.h>
#include <Print.h>

namespace coredump {
  boolean init();
  boolean checkForCoredump();
  void logCoredumpSummary();
  boolean writeCoredumpToFile();
  boolean writeCoredumpToStream(Print* p);
  boolean getCoredumpSize(size_t* out_addr, size_t* out_size);
  boolean eraseCoredump();

}