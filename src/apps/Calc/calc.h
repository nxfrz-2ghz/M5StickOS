#pragma once

#include "../App.h"
#include <vector>

class CalcApp : public App {
public:
    const char* GetName() const override { return "Calc"; }
    bool Loop() override;

private:
    void updateScreen();
    char inputKeyboard();

    // Раньше это были статические переменные уровня файла — единственный
    // на всю прошивку экземпляр состояния, который жил вечно и требовал
    // ручного сброса в Setup() при каждом повторном запуске приложения.
    // Теперь это обычные поля объекта: лаунчер создаёт новый CalcApp при
    // каждом запуске (см. main.cpp), поэтому они и так каждый раз получают
    // свои значения по умолчанию — отдельный сброс в Setup() не нужен.
    std::vector<char> data_array;
    int selectedIndex = 10;
    int lastIndex = 10;
};
