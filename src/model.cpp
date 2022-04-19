#include <model.h>

Model::Model(modelUpdatedEvt_t _modelUpdatedEvt) {
  this->temperature = NaN;
  this->humidity = NaN;
  this->co2 = 0;
  this->modelUpdatedEvt = _modelUpdatedEvt;
}

Model::~Model() {}

void Model::updateModel(uint16_t _co2, float _temperature, float _humidity) {
  this->co2 = _co2;
  this->temperature = _temperature;
  this->humidity = _humidity;
  modelUpdatedEvt(M_CO2 || M_TEMPERATURE || M_HUMIDITY);
}

void Model::updateModel(float temperature, float humidity, uint16_t pressure, uint16_t iaq) {
  modelUpdatedEvt(M_TEMPERATURE || M_HUMIDITY || M_PRESSURE || M_IAQ);
}

void Model::updateModel(uint16_t pm0_5, uint16_t pm1_0, uint16_t pm2_5, uint16_t pm4, uint16_t pm10) {
  modelUpdatedEvt(M_PM0_5 || M_PM1_0 || M_PM2_5 || M_PM4 || M_PM10);
}

uint16_t Model::getCo2() {
  return this->co2;
}

float Model::getTemperature() {
  return this->temperature;
}

float Model::getHumidity() {
  return this->humidity;
}

uint16_t Model::getPressure() {
  return this->pressure;
}

uint16_t Model::getIAQ() {
  return this->iaq;
}

uint16_t Model::getPM0_5() {
  return this->pm0_5;
}

uint16_t Model::getPM1_0() {
  return this->pm10;
}

uint16_t Model::getPM2_5() {
  return this->pm2_5;
}

uint16_t Model::getPM4() {
  return this->pm4;
}

uint16_t Model::getPM10() {
  return this->pm10;
}

