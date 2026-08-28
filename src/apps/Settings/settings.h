#pragma once

#include "../App.h"

class SettingsApp : public App {
public:
    const char* GetName() const override { return "Settings"; }
    bool Loop() override { return true; }
};

extern SettingsApp settingsApp;
