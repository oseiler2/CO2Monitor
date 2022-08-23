#ifndef _MENUITEM_H
#define _MENUITEM_H

#include <globals.h>

typedef char const* (*getLabelForSelectionCallback_t)(uint8_t);
typedef void (*actionMenuItemCallback_t)(uint8_t  selection);

const char* renderSelectionAsLabel(uint8_t index);

class MenuItem {
public:
  MenuItem(char const* name, uint8_t minValue, uint8_t maxValue, uint8_t steps, uint8_t  initialSelection, getLabelForSelectionCallback_t getLabelForSelectionCallback, actionMenuItemCallback_t actionMenuItemCallback = nullptr);
  MenuItem(char const* name, uint8_t minValue, uint8_t maxValue, uint8_t steps, uint8_t  initialSelection, const char* const* labels, actionMenuItemCallback_t actionMenuItemCallback = nullptr);
  MenuItem(char const* name, const char* const* labels, actionMenuItemCallback_t actionMenuItemCallback);
  MenuItem(char const* name, actionMenuItemCallback_t actionMenuItemCallback);
  ~MenuItem();

  void selectNext();
  void selectPrev();
  void action();

  uint8_t getSelection();
  void setSelection(uint8_t _selection);
  const char* getCurrentLabel();
  char const* getName();
  boolean isActionMenuItem();


private:
  char const* name;
  const char* const* labels;
  uint8_t offset;
  uint16_t range;
  uint8_t steps;
  uint8_t currentSelection;
  getLabelForSelectionCallback_t getLabelForSelectionCallback;
  actionMenuItemCallback_t actionMenuItemCallback;
};


#endif