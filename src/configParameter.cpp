#include <configParameter.h>

// Local logging tag
static const char TAG[] = __FILE__;

// -------------------- generic -------------------
template <typename C, typename T>
ConfigParameter<C, T>::ConfigParameter(const char* _id, const char* _label, T C::* _valuePtr, uint8_t _maxStrLen) {
  this->id = _id;
  this->label = _label;
  this->valuePtr = _valuePtr;
  this->maxStrLen = _maxStrLen;
}

template <typename C, typename T>
uint8_t ConfigParameter<C, T>::getMaxStrLen(void) {
  return this->maxStrLen;
}

template <typename C, typename T>
const char* ConfigParameter<C, T>::getId() {
  return this->id;
}

template <typename C, typename T>
const char* ConfigParameter<C, T>::getLabel() {
  return this->label;
}

template <typename C, typename T>
void ConfigParameter<C, T>::save(C& config, const char* str) {
  this->parse(config, this->valuePtr, str);
}

template <typename C, typename T>
bool ConfigParameter<C, T>::isNumber() {
  return false;
}

template <typename C, typename T>
bool ConfigParameter<C, T>::isBoolean() {
  return false;
}

template <typename C, typename T>
T* ConfigParameter<C, T>::getValuePtr() {
  return this->valuePtr;
}

template <typename C, typename T>
void ConfigParameter<C, T>::getMinimum(char* str) { str[0] = 0; }

template <typename C, typename T>
void ConfigParameter<C, T>::getMaximum(char* str) { str[0] = 0; }

template <typename C, typename T>
String ConfigParameter<C, T>::toString(const C config) {
  char buffer[this->getMaxStrLen()] = { 0 };
  this->print(config, buffer);
  return String(buffer);
}

// -------------------- number -------------------
template <typename C, typename T>
NumberConfigParameter<C, T>::NumberConfigParameter(const char* _id, const char* _label, T C::* _valuePtr, T _defaultValue, uint8_t _maxStrLen, T _min, T _max) :
  ConfigParameter<C, T>(_id, _label, _valuePtr, _maxStrLen) {
  this->minValue = _min;
  this->maxValue = _max;
  _ASSERT(_min <= _defaultValue);
  _ASSERT(_defaultValue <= _max);
  this->defaultValue = _defaultValue;
}

template <typename C, typename T>
NumberConfigParameter<C, T>::~NumberConfigParameter() {};

template <typename C, typename T>
bool NumberConfigParameter<C, T>::isNumber() {
  return true;
}

template <typename C, typename T>
void NumberConfigParameter<C, T>::getMinimum(char* str) {
  snprintf(str, 6, "%d", this->minValue);
}

template <typename C, typename T>
void NumberConfigParameter<C, T>::getMaximum(char* str) {
  snprintf(str, 6, "%d", this->maxValue);
}

template <typename C, typename T>
void NumberConfigParameter<C, T>::setToDefault(C& config) {
  config.*(this->valuePtr) = this->defaultValue;
}

template <typename C, typename T>
void NumberConfigParameter<C, T>::toJson(const C config, DynamicJsonDocument* doc) {
  (*doc)[this->getId()] = config.*(this->valuePtr);
};

template <typename C, typename T>
void NumberConfigParameter<C, T>::fromJson(C& config, DynamicJsonDocument* doc) {
  config.*(this->valuePtr) = (*doc)[this->getId()] | this->defaultValue;
};

// -------------------- uint8_t -------------------
template <typename C>
Uint8ConfigParameter<C>::Uint8ConfigParameter(const char* _id, const char* _label, uint8_t C::* _valuePtr, uint8_t _defaultValue, uint8_t min, uint8_t max) :
  NumberConfigParameter<C, uint8_t>(_id, _label, _valuePtr, _defaultValue, 4, min, max) {}

template <typename C>
Uint8ConfigParameter<C>::~Uint8ConfigParameter() {};

template <typename C>
void Uint8ConfigParameter<C>::print(const C config, char* str) {
  sprintf(str, "%u", config.*(this->valuePtr));
}

template <typename C>
void Uint8ConfigParameter<C>::parse(C& config, uint8_t C::* valuePtr, const char* str) {
  config.*(this->valuePtr) = (uint8_t)atoi(str);
}

// -------------------- uint16_t -------------------
template <typename C>
Uint16ConfigParameter<C>::Uint16ConfigParameter(const char* _id, const char* _label, uint16_t C::* _valuePtr, uint16_t _defaultValue, uint16_t min, uint16_t max) :
  NumberConfigParameter<C, uint16_t>(_id, _label, _valuePtr, _defaultValue, 6, min, max) {}

template <typename C>
Uint16ConfigParameter<C>::~Uint16ConfigParameter() {}

template <typename C>
void Uint16ConfigParameter<C>::print(const C config, char* str) {
  sprintf(str, "%u", config.*(this->valuePtr));
}

template <typename C>
void Uint16ConfigParameter<C>::parse(C& config, uint16_t C::* valuePtr, const char* str) {
  config.*(this->valuePtr) = (uint16_t)atoi(str);
}

// -------------------- bool -------------------
template <typename C>
BooleanConfigParameter<C>::BooleanConfigParameter(const char* _id, const char* _label, bool C::* _valuePtr, bool _defaultValue)
  :ConfigParameter<C, bool>(_id, _label, _valuePtr, 6) {
  this->defaultValue = _defaultValue;
}

template <typename C>
BooleanConfigParameter<C>::~BooleanConfigParameter() {}

template <typename C>
bool BooleanConfigParameter<C>::isBoolean() {
  return true;
}

template <typename C>
void BooleanConfigParameter<C>::print(const C config, char* str) {
  sprintf(str, "%s", config.*(this->valuePtr) ? "true" : "false");
}

template <typename C>
void BooleanConfigParameter<C>::parse(C& config, bool C::* valuePtr, const char* str) {
  config.*(this->valuePtr) = strcmp("true", str) == 0 || strcmp("on", str) == 0;
}

template <typename C>
void BooleanConfigParameter<C>::setToDefault(C& config) {
  config.*(this->valuePtr) = this->defaultValue;
}

template <typename C>
void BooleanConfigParameter<C>::toJson(const C config, DynamicJsonDocument* doc) {
  (*doc)[this->getId()] = config.*(this->valuePtr);
};

template <typename C>
void BooleanConfigParameter<C>::fromJson(C& config, DynamicJsonDocument* doc) {
  config.*(this->valuePtr) = (*doc)[this->getId()] | this->defaultValue;
};

// -------------------- char* -------------------
template <typename C>
CharArrayConfigParameter<C>::CharArrayConfigParameter(const char* _id, const char* _label, char C::* _valuePtr, const char* _defaultValue, uint8_t _maxLen)
  :ConfigParameter<C, char>(_id, _label, _valuePtr, _maxLen) {
  _ASSERT(strlen(_defaultValue) <= _maxLen);
  this->defaultValue = _defaultValue;
}
template <typename C>
CharArrayConfigParameter<C>::~CharArrayConfigParameter() {}

template <typename C>
void CharArrayConfigParameter<C>::print(const C config, char* str) {
  sprintf(str, "%s", (char*)&(config.*(this->valuePtr)));
}

template <typename C>
void CharArrayConfigParameter<C>::parse(C& config, char C::* valuePtr, const char* str) {
  strncpy((char*)&(config.*(this->valuePtr)), str, min(strlen(str), (size_t)(this->maxStrLen - 1)));
  ((char*)&(config.*(this->valuePtr)))[min(strlen(str), (size_t)(this->maxStrLen - 1))] = 0x00;
}

template <typename C>
void CharArrayConfigParameter<C>::setToDefault(C& config) {
  this->save(config, defaultValue);
}

template <typename C>
void CharArrayConfigParameter<C>::toJson(const C config, DynamicJsonDocument* doc) {
  (*doc)[this->getId()] = (char*)&(config.*(this->valuePtr));
};

template <typename C>
void CharArrayConfigParameter<C>::fromJson(C& config, DynamicJsonDocument* doc) {
  const char* fromJson = (*doc)[this->getId()] | this->defaultValue;
  strncpy((char*)&(config.*(this->valuePtr)), fromJson, min(strlen(fromJson), (size_t)(this->maxStrLen - 1)));
  ((char*)&(config.*(this->valuePtr)))[min(strlen(fromJson), (size_t)(this->maxStrLen - 1))] = 0x00;
};

template class Uint8ConfigParameter<Config>;
template class Uint16ConfigParameter<Config>;
template class BooleanConfigParameter<Config>;
template class CharArrayConfigParameter<Config>;