#pragma once

#include <vector>
#include <Arduino.h>

class GUI {
public:
    static const int textLinesPerPage = 7;
    static const int textCharsPerLine = 32;

    static const int listLinesPerPage = 5;
    static const int listCharsPerLine = 24;

    static void clear();
    static void bigText(const char* text);
    static void text(const char* text);
    static void line(const char* text, int lineNumber);
    static void list(const char* title, const std::vector<String> &list, int selectedIndex, bool enableWrap = false);
    static void progressBar(float progress, int barHeight = 3);
    static int updateMenuSelection(int index, const int max, const int updateDelay = 200);
};
