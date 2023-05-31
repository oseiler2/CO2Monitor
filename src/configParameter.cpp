#include <configParameter.h>

// Local logging tag
static const char TAG[] = __FILE__;

// -------------------- generic -------------------
template <typename T>
ConfigParameter<T>::ConfigParameter(const char* _id, const char* _label, T* _valuePtr, uint8_t _maxStrLen) {
  this->id = _id;
  this->label = _label;
  this->valuePtr = _valuePtr;
  this->maxStrLen = _maxStrLen;
}

template <typename T>
uint8_t ConfigParameter<T>::getMaxStrLen(void) {
  return this->maxStrLen;
}

template <typename T>
const char* ConfigParameter<T>::getId() {
  return this->id;
}

template <typename T>
const char* ConfigParameter<T>::getLabel() {
  return this->label;
}

template <typename T>
void ConfigParameter<T>::save(const char* str) {
  this->parse(this->valuePtr, str);
}

template <typename T>
bool ConfigParameter<T>::isNumber() {
  return false;
}

template <typename T>
bool ConfigParameter<T>::isBoolean() {
  return false;
}

template <typename T>
T* ConfigParameter<T>::getValuePtr() {
  return this->valuePtr;
}

template <typename T>
void ConfigParameter<T>::getMinimum(char* str) { str[0] = 0; }

template <typename T>
void ConfigParameter<T>::getMaximum(char* str) { str[0] = 0; }


// -------------------- number -------------------
template <typename T>
NumberConfigParameter<T>::NumberConfigParameter(const char* _id, const char* _label, T* _valuePtr, uint8_t _maxStrLen, T _min, T _max) :ConfigParameter<T>(_id, _label, _valuePtr, _maxStrLen) {
  this->minValue = _min;
  this->maxValue = _max;
}

template <typename T>
NumberConfigParameter<T>::~NumberConfigParameter() {};

template <typename T>
bool NumberConfigParameter<T>::isNumber() {
  return true;
}

template <typename T>
void NumberConfigParameter<T>::getMinimum(char* str) {
  snprintf(str, 6, "%d", this->minValue);
}

template <typename T>
void NumberConfigParameter<T>::getMaximum(char* str) {
  snprintf(str, 6, "%d", this->maxValue);
}

// -------------------- uint8_t -------------------
Uint8ConfigParameter::Uint8ConfigParameter(const char* _id, const char* _label, uint8_t* _valuePtr, uint8_t min, uint8_t max) :NumberConfigParameter<uint8_t>(_id, _label, _valuePtr, 4, min, max) {}
Uint8ConfigParameter::~Uint8ConfigParameter() {};

void Uint8ConfigParameter::print(char* str) {
  sprintf(str, "%u", *valuePtr);
}

void Uint8ConfigParameter::parse(uint8_t* valuePtr, const char* str) {
  *valuePtr = (uint8_t)atoi(str);
}


// -------------------- uint16_t -------------------
Uint16ConfigParameter::Uint16ConfigParameter(const char* _id, const char* _label, uint16_t* _valuePtr, uint16_t min, uint16_t max) :NumberConfigParameter<uint16_t>(_id, _label, _valuePtr, 6, min, max) {}
Uint16ConfigParameter::~Uint16ConfigParameter() {}

void Uint16ConfigParameter::print(char* str) {
  sprintf(str, "%u", *valuePtr);
}

void Uint16ConfigParameter::parse(uint16_t* valuePtr, const char* str) {
  *valuePtr = (uint16_t)atoi(str);
}

// -------------------- bool -------------------
BooleanConfigParameter::BooleanConfigParameter(const char* _id, const char* _label, bool* _valuePtr) :ConfigParameter(_id, _label, _valuePtr, 6) {}
BooleanConfigParameter::~BooleanConfigParameter() {}

bool BooleanConfigParameter::isBoolean() {
  return true;
}

void BooleanConfigParameter::print(char* str) {
  sprintf(str, "%s", *valuePtr ? "true" : "false");
}

void BooleanConfigParameter::parse(bool* valuePtr, const char* str) {
  *valuePtr = strcmp("true", str) == 0 || strcmp("on", str) == 0;
}


// -------------------- char* -------------------
CharArrayConfigParameter::CharArrayConfigParameter(const char* _id, const char* _label, char* _valuePtr, uint8_t _maxLen) :ConfigParameter(_id, _label, _valuePtr, _maxLen) {}
CharArrayConfigParameter::~CharArrayConfigParameter() {}

void CharArrayConfigParameter::print(char* str) {
  sprintf(str, "%s", valuePtr);
}

void CharArrayConfigParameter::parse(char* valuePtr, const char* str) {
  strncpy(valuePtr, str, min(strlen(str), (size_t)(this->maxStrLen - 1)));
  valuePtr[min(strlen(str), (size_t)(this->maxStrLen - 1))] = 0x00;
}

