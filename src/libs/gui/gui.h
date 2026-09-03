#pragma once

#include <vector>
#include <Arduino.h>

const int linesPerPage = 5;
const int charsPerLine = 18;

void displayClear();

void displayBigText(const char* text);
void displayText(const char* text);

void displayList(const char* title, const std::vector<String> &list, int selectedIndex = -1);
void displayProgressBar(float progress, int barHeight = 3);

int updateMenuSelection(int index, const int max);
int updateMenuSelectionFast(int index, const int max, const int updateDelay = 50);
