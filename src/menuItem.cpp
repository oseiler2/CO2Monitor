#include <config.h>
#include <Arduino.h>
#include <menuItem.h>

// Local logging tag
static const char TAG[] = __FILE__;

char buf[5];

MenuItem::MenuItem(char const* _name, uint8_t _minValue, uint8_t _maxValue, uint8_t _steps, uint8_t  _initialSelection, getLabelForSelectionCallback_t _getLabelForSelectionCallback, actionMenuItemCallback_t _actionMenuItemCallback) {
  _ASSERT(_maxValue >= _minValue);
  _ASSERT((_steps <= _maxValue - _minValue + 1) && (steps >= 0));
  _ASSERT(_minValue <= _initialSelection && _initialSelection <= _maxValue);
  this->name = _name;
  this->offset = _minValue;
  this->range = _maxValue - _minValue + 1;
  this->steps = _steps;
  this->currentSelection = _initialSelection - offset;
  this->labels = nullptr;
  this->getLabelForSelectionCallback = _getLabelForSelectionCallback;
  this->actionMenuItemCallback = _actionMenuItemCallback;
  //  ESP_LOGD(TAG, "name %s, min %u, max %u, steps %u, initialSelection %u, offset %u, range %u", _name, _minValue, _maxValue, _steps, _initialSelection, offset, range);
}

MenuItem::MenuItem(char const* _name, uint8_t _minValue, uint8_t _maxValue, uint8_t _steps, uint8_t  _initialSelection, const char* const* _labels, actionMenuItemCallback_t _actionMenuItemCallback) {
  _ASSERT(_maxValue >= _minValue);
  _ASSERT((_steps <= _maxValue - _minValue + 1) && (steps >= 0));
  _ASSERT(_minValue <= _initialSelection && _initialSelection <= _maxValue);
  this->name = _name;
  this->offset = _minValue;
  this->range = _maxValue - _minValue + 1;
  this->steps = _steps;
  this->currentSelection = _initialSelection - offset;
  this->labels = _labels;
  this->getLabelForSelectionCallback = nullptr;
  this->actionMenuItemCallback = _actionMenuItemCallback;
  //  ESP_LOGD(TAG, "name %s, min %u, max %u, steps %u, initialSelection %u, offset %u, range %u", _name, _minValue, _maxValue, _steps, _initialSelection, offset, range);
}

MenuItem::MenuItem(char const* _name, const char* const* _labels, actionMenuItemCallback_t _actionMenuItemCallback) {
  _ASSERT(_actionMenuItemCallback != nullptr);
  this->name = _name;
  this->offset = 0;
  this->range = 1;
  this->steps = 0;
  this->currentSelection = 0;
  this->labels = _labels;
  this->getLabelForSelectionCallback = nullptr;
  this->actionMenuItemCallback = _actionMenuItemCallback;
  //  ESP_LOGD(TAG, "name %s, steps %u, currentSelection %u, offset %u, range %u", _name, steps, currentSelection, offset, range);
}

MenuItem::MenuItem(char const* _name, actionMenuItemCallback_t _actionMenuItemCallback) {
  _ASSERT(_actionMenuItemCallback != nullptr);
  this->name = _name;
  this->offset = 0;
  this->range = 1;
  this->steps = 0;
  this->currentSelection = 0;
  this->labels = nullptr;
  this->getLabelForSelectionCallback = nullptr;
  this->actionMenuItemCallback = _actionMenuItemCallback;
  //  ESP_LOGD(TAG, "name %s, steps %u, currentSelection %u, offset %u, range %u", _name, steps, currentSelection, offset, range);
}

MenuItem::~MenuItem() {}

void MenuItem::action() {
  if (actionMenuItemCallback != nullptr && actionMenuItemCallback != NULL) {
    actionMenuItemCallback(currentSelection + offset);
  }
}

boolean MenuItem::isActionMenuItem() {
  return (range == 1 && actionMenuItemCallback != nullptr);
}

void MenuItem::selectNext() {
  this->currentSelection = (this->currentSelection + this->steps) % this->range;
}

void MenuItem::selectPrev() {
  this->currentSelection = (this->currentSelection + this->range - this->steps) % this->range;
}

uint8_t MenuItem::getSelection() {
  return this->currentSelection + this->offset;
}

void MenuItem::setSelection(uint8_t _selection) {
  if (_selection < this->offset || _selection >= this->offset + this->range) return;
  this->currentSelection = _selection - this->offset;
}

const char* MenuItem::getCurrentLabel() {
  if (getLabelForSelectionCallback == nullptr) {
    if (labels == nullptr) {
      return this->name;
    } else {
      return labels[this->currentSelection + this->offset];
    }
  } else {
    return this->getLabelForSelectionCallback(this->currentSelection + this->offset);
  }
}

char const* MenuItem::getName() {
  return this->name;
}

const char* renderSelectionAsLabel(uint8_t index) {
  sprintf(buf, "%u", index);
  return buf;
}

