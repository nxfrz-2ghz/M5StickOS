#pragma once

#include "../App.h"

class HomeApp : public App {
public:
    const char* GetName() const override { return "Home"; }
    bool Loop() override;
    bool AutoStart() const override { return true; }
};
