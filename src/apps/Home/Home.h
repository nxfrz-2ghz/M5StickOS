#pragma once

#include "../App.h"

class HomeApp : public App {
public:
    static const char* GetName() { return "Home"; }
    static bool AutoStart() { return true; }

    bool Loop() override;
};
