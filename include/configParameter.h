#ifndef _CONFIG_PARAMETER_H
#define _CONFIG_PARAMETER_H

#include <globals.h>
#include <config.h>

class ConfigParameterBase {
public:
  virtual uint8_t getMaxStrLen(void) = 0;
  virtual const char* getId() = 0;
  virtual const char* getLabel() = 0;
  virtual void print(char* str) = 0;
  virtual void save(const char* str) = 0;
  virtual bool isNumber() = 0;
  virtual bool isBoolean() = 0;
  virtual void getMinimum(char* str) = 0;
  virtual void getMaximum(char* str) = 0;
};

template <typename T>
class ConfigParameter :public ConfigParameterBase {
public:
  ConfigParameter(const char* id, const char* label, T* valuePtr, uint8_t maxStrLen);
  virtual ~ConfigParameter() = default;
  uint8_t getMaxStrLen(void) override;
  const char* getId() override;
  const char* getLabel() override;
  void save(const char* str) override;
  bool isNumber() override;
  bool isBoolean() override;
  T* getValuePtr();
  void getMinimum(char* str) override;
  void getMaximum(char* str) override;

protected:
  virtual void parse(T* valuePtr, const char* str) = 0;
  const char* id;
  const char* label;
  T* valuePtr;
  uint8_t maxStrLen;
};

template <typename T>
class NumberConfigParameter :public ConfigParameter<T> {
public:
  NumberConfigParameter(const char* id, const char* label, T* valuePtr, uint8_t maxStrLen, T min, T max);
  ~NumberConfigParameter();
  bool isNumber() override;
  void getMinimum(char* str) override;
  void getMaximum(char* str) override;
protected:
  T minValue;
  T maxValue;
};

class Uint8ConfigParameter :public NumberConfigParameter<uint8_t> {
public:
  Uint8ConfigParameter(const char* id, const char* label, uint8_t* valuePtr, uint8_t min = 0, uint8_t max = 255);
  ~Uint8ConfigParameter();
  void print(char* str) override;
protected:
  void parse(uint8_t* valuePtr, const char* str) override;
};

class Uint16ConfigParameter :public NumberConfigParameter<uint16_t> {
public:
  Uint16ConfigParameter(const char* id, const char* label, uint16_t* valuePtr, uint16_t min = 0, uint16_t max = 65535);
  ~Uint16ConfigParameter();
  void print(char* str) override;
protected:
  void parse(uint16_t* valuePtr, const char* str) override;
};

class BooleanConfigParameter :public ConfigParameter<bool> {
public:
  BooleanConfigParameter(const char* id, const char* label, bool* valuePtr);
  ~BooleanConfigParameter();
  void print(char* str) override;
  bool isBoolean() override;
protected:
  void parse(bool* valuePtr, const char* str) override;
};

class CharArrayConfigParameter :public ConfigParameter<char> {
public:
  CharArrayConfigParameter(const char* id, const char* label, char* valuePtr, uint8_t maxLen);
  ~CharArrayConfigParameter();
  void print(char* str) override;
protected:
  void parse(char* valuePtr, const char* str) override;
};

#endif