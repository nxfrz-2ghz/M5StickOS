#pragma once

#include "../App.h"

class CalcApp : public App {
public:
    const char* GetName() const override { return "Calc"; }
    void Setup() override;
    bool Loop() override;
};

extern CalcApp calcApp;
