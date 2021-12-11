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
  modelUpdatedEvt();
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
