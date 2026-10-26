#pragma once

#include "../app.h"
#include <vector>
#include <Arduino.h>

class CalcApp : public App {
public:
    static const char* GetName() { return "Calc"; }

    bool Loop() override;

private:
    void reset();
    void updateScreen();
    void applyKey(char key);
    void applyUnary(char key);
    void calculateResult();
    String trimNumber(float value);
    bool runFunction(int index);

    static const unsigned long DOUBLE_PRESS_MS = 500;

    std::vector<String> keyboardData = {
        "= | 0", "< | 1", ". | 2", "! | 3", "+ | 4", "- | 5", "* | 6",
        "/ | 7", "^ | 8", "Q | 9", "M | sin", "R | cos", "E | tg"
    };

    int selectedIndex = 0; // "="
    int lastIndex = -1;

    // Отложенное одиночное нажатие
    bool pending = false;
    unsigned long pendingTime = 0;
    int pendingIndex = 0;

    bool isSecond = false;
    bool showingResult = false;
    bool hasError = false;

    String fisrtInp = "0";
    String secondInp = "0";
    String op = "";
    double result = 0;
};