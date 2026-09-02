#pragma once

#include "../App.h"

class SettingsApp : public App {
public:
    static const char* GetName() { return "Settings"; }

    bool Loop() override;
};
