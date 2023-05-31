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

};

template <typename T>
class ConfigParameter :public ConfigParameterBase {
public:
  ConfigParameter(const char* id, const char* label, T* valuePtr, uint8_t maxStrLen);
  virtual ~ConfigParameter() = default;
  virtual void parse(T* valuePtr, const char* str) = 0;
  uint8_t getMaxStrLen(void);
  const char* getId();
  const char* getLabel();
  virtual void print(char* str) = 0;
  void save(const char* str);

protected:
  const char* id;
  const char* label;
  T* valuePtr;
  uint8_t maxStrLen;
};

class Uint8ConfigParameter :public ConfigParameter<uint8_t> {
public:
  Uint8ConfigParameter(const char* id, const char* label, uint8_t* valuePtr);
  ~Uint8ConfigParameter();
  void parse(uint8_t* valuePtr, const char* str) override;
  void print(char* str) override;
};

class Uint16ConfigParameter :public ConfigParameter<uint16_t> {
public:
  Uint16ConfigParameter(const char* id, const char* label, uint16_t* valuePtr);
  ~Uint16ConfigParameter();
  void parse(uint16_t* valuePtr, const char* str) override;
  void print(char* str) override;
};

class BooleanConfigParameter :public ConfigParameter<bool> {
public:
  BooleanConfigParameter(const char* id, const char* label, bool* valuePtr);
  ~BooleanConfigParameter();
  void parse(bool* valuePtr, const char* str) override;
  void print(char* str) override;
};

class CharArrayConfigParameter :public ConfigParameter<char> {
public:
  CharArrayConfigParameter(const char* id, const char* label, char* valuePtr, uint8_t maxLen);
  ~CharArrayConfigParameter();
  void parse(char* valuePtr, const char* str) override;
  void print(char* str) override;
};

#endif