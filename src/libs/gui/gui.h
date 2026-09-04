#pragma once

#include <vector>
#include <Arduino.h>

const int displayTextLinesPerPage = 5;
const int displayTextCharsPerLine = 18;

const int listLinesPerPage = 5;
const int listCharsPerLine = 24;

void displayClear();

void displayBigText(const char* text);
void displayText(const char* text);

void displayList(const char* title, const std::vector<String> &list, int selectedIndex, bool enableWrap = false);
void displayProgressBar(float progress, int barHeight = 3);

int updateMenuSelection(int index, const int max);
int updateMenuSelectionFast(int index, const int max, const int updateDelay = 50);
