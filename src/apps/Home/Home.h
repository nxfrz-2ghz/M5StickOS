#pragma once

#include "../app.h"

class HomeApp : public App {
public:
    static const char* GetName() { return coreTemp; }
    static const bool AutoStart() { return true; }

    bool Loop() override;
private:
    static const unsigned long tempUpdInterval = 1000;
    static char coreTemp[16];
    static unsigned long lastTempUpd;
};
