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

// -------------------- uint8_t -------------------
Uint8ConfigParameter::Uint8ConfigParameter(const char* _id, const char* _label, uint8_t* _valuePtr) :ConfigParameter(_id, _label, _valuePtr, 4) {}
Uint8ConfigParameter::~Uint8ConfigParameter() {};

void Uint8ConfigParameter::print(char* str) {
  sprintf(str, "%u", *valuePtr);
}

void Uint8ConfigParameter::parse(uint8_t* valuePtr, const char* str) {
  *valuePtr = (uint8_t)atoi(str);
}


// -------------------- uint16_t -------------------
Uint16ConfigParameter::Uint16ConfigParameter(const char* _id, const char* _label, uint16_t* _valuePtr) :ConfigParameter(_id, _label, _valuePtr, 6) {}
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

void BooleanConfigParameter::print(char* str) {
  sprintf(str, "%s", *valuePtr ? "true" : "false");
}

void BooleanConfigParameter::parse(bool* valuePtr, const char* str) {
  *valuePtr = strcmp("true", str) == 0;
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

