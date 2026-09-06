#pragma once

#include "../app.h"

class HomeApp : public App {
public:
    static const char* GetName() { return "Home"; }
    static const bool AutoStart() { return true; }

    bool Loop() override;
};
