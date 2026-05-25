#pragma once

#include <vector>

void displayBigText(const String &text);
void displayText(const String &text);
void displayList(const String &title, std::vector<String> list, int selectedIndex = -1);
int updateMenuSelection(int index, const int max);
