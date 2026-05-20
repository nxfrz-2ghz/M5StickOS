#ifndef GUI_H
#define GUI_H

#include "M5StickCPlus2.h"
#include "btnC.h"
#include <vector>

void displayBigText(const String &text);
void displayText(const String &text);
void displayList(const String &title, std::vector<String> list, int selectedIndex = -1);
int updateMenuSelection(int index, const int max);

#endif
