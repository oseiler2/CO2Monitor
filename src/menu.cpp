#include <config.h>
#include <Arduino.h>
#include <menu.h>
#include <configManager.h>
#include <lcd.h>
#include <power.h>
#include <wifiManager.h>
#include <scd40.h>
#include <neopixel.h>
#include <buzzer.h>
#include <scd40.h>
#include <bme680.h>

// Local logging tag
static const char TAG[] = __FILE__;

extern LCD* lcd;
extern Neopixel* neopixel;
extern Buzzer* buzzer;
extern SCD40* scd40;
extern BME680* bme680;

extern bool hasBuzzer;
extern bool hasNeoPixel;

extern void prepareOta();
extern void calibrateCo2SensorCallback(uint16_t co2Reference);
extern void updateMessage(char const* msg);
extern void setPriorityMessage(char const* msg);
extern void clearPriorityMessage();

namespace Menu {

  const char* const miBuzzerLabels[] = {
    "Off",
    "On level\nchange",
    "Always"
  };

  const char* const miSleepLabels[] = {
    "Display on\nLED on",
    "Display on\nLED off",
    "Display off\nLED on",
    "Display off\nLED off"
  };

  const char* const miCalibrationLabels[] = { "Perform\ncalibration" };

  const char* const miAccessPointLabels[] = { "Access\npoint" };

  const char* const miGoToSleepLabels[] = { "Go to\nsleep" };

  void setBuzzerAction(uint8_t selection) {
    ESP_LOGI(TAG, "setBuzzerAction %u", selection);
    config.buzzerMode = getBuzzerModeFromUint(selection);
    saveConfiguration(config);
    model->configurationChanged();
  }

  void setBrightnessAction(uint8_t selection) {
    ESP_LOGI(TAG, "setBrightnessAction %u", selection);
    config.brightness = selection;
    saveConfiguration(config);
    model->configurationChanged();
  }

  void setSleepModeAction(uint8_t selection) {
    ESP_LOGI(TAG, "setSleepModeAction %u", selection);
    config.sleepModeOledLed = getSleepModeOledLedFromUint(selection);
    saveConfiguration(config);
  }

  void doCalibrateAction(uint8_t selection) {
    ESP_LOGI(TAG, "Perform calibration %u", selection);
    calibrateCo2SensorCallback(420);
  }

  void startAPAction(uint8_t selection) {
    ESP_LOGI(TAG, "startAPAction %u", selection);
    if (Power::getPowerMode() == USB) {
      digitalWrite(LED_PIN, LOW);
      prepareOta();
      WifiManager::startCaptivePortal();
    }
  }

  void goToSleepAction(uint8_t selection) {
    ESP_LOGI(TAG, "goToSleepAction()");
    if (neopixel) neopixel->prepareToSleep();
    Power::setPowerMode(BATTERY);
    if (scd40) scd40->setSampleRate(LP_PERIODIC);
    if (bme680) bme680->setSampleRate(ULP);
    Power::deepSleep(30);
  }

  void powerDownAction(uint8_t selection) {
    ESP_LOGI(TAG, ">>>> Battery critial - turning off !");
    if (hasBuzzer && buzzer) buzzer->alert();
    if (hasNeoPixel && neopixel) neopixel->off();
    if (scd40) scd40->shutdown();
    if (bme680) bme680->shutdown();
    Power::powerDown();
  }

  void noAction(uint8_t selection) {
    ESP_LOGI(TAG, "NoAction %u", selection);
  }

  int8_t menuLevel = -1;
  const uint8_t menuItemSize = 8;
  uint8_t currentMenuItem = 0;
  MenuItem* brightnessMenuItem = new MenuItem("Brightness", 0, 255, 5, config.brightness, renderSelectionAsLabel, setBrightnessAction);
  MenuItem* buzzerMenuItem = new MenuItem("Buzzer", 0, 2, 1, 0, miBuzzerLabels, setBuzzerAction);
  MenuItem* sleepModeMenuItem = new MenuItem("Sleep mode", 0, 3, 1, 0, miSleepLabels, setSleepModeAction);
  MenuItem* menuItems[] = {
   new MenuItem("Back", noAction),
   brightnessMenuItem,
   buzzerMenuItem,
   sleepModeMenuItem,
   new MenuItem("Calibrate", miCalibrationLabels, doCalibrateAction),
   new MenuItem("Start AP", miAccessPointLabels, startAPAction),
   new MenuItem("Sleep...", miGoToSleepLabels, goToSleepAction),
   new MenuItem("Power down", powerDownAction)
  };

  void showMenu() {
    if (menuLevel < 0) {
      lcd->quitMenu();
    } else {
      lcd->showMenu(menuItems[currentMenuItem]->getName(), menuLevel == 0 ? "" : menuItems[currentMenuItem]->getCurrentLabel());
    }
  }

  void button1Pressed() {
    /*if (menuLevel == 0 && menuItems[currentMenuItem]->isActionMenuItem()) {
      menuItems[currentMenuItem]->action();
      menuLevel--;
    } else*/ if (menuLevel < 1) {
      if (menuLevel == -1) {
        brightnessMenuItem->setSelection(config.brightness);
        buzzerMenuItem->setSelection(config.buzzerMode);
        sleepModeMenuItem->setSelection(config.sleepModeOledLed);
        currentMenuItem = 0;
      }
      menuLevel++;
      ESP_LOGI(TAG, ">>> Select!");
    } else if (menuLevel == 1) {
      lcd->quitMenu();
      menuItems[currentMenuItem]->action();
      menuLevel = -1;
      return;
    }
    showMenu();
  }

  void button2Pressed() {
    if (menuLevel > -1) {
      menuLevel--;
      ESP_LOGI(TAG, "<<< Back!");
      showMenu();
    }
  }

  void button3Pressed() {
    if (menuLevel == 0) {
      currentMenuItem = (currentMenuItem + 1) % menuItemSize;
      showMenu();
    } else {
      if (menuLevel == 1) {
        menuItems[currentMenuItem]->selectNext();
        showMenu();
      }
    }
  }

  void button4Pressed() {
    if (menuLevel == 0) {
      currentMenuItem = (currentMenuItem + menuItemSize - 1) % menuItemSize;
      showMenu();
    } else {
      if (menuLevel == 1) {
        menuItems[currentMenuItem]->selectPrev();
        showMenu();
      }
    }
  }
}