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
    void calculateResult();
    String trimNumber(float value);

    std::vector<String> keyboardData = {
        "9", "8", "7", "6", "5", "4", "3", "2", "1", "0",
        "=", "<", ".", "!", "+", "-", "*", "/", "^", "Q",
        "M", "R", "E"
    };

    int selectedIndex = 10; // "="
    int lastIndex = -1;

    bool numnum = false;  // ввод первого или второго числа
    bool showingResult = false;
    bool hasError = false;

    String firstinp = "0";
    String secondinp = "0";
    String op = "";
    float result = 0;
};