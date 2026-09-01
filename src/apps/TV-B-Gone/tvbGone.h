#pragma once

#include "../App.h"
#include "worldIrCodes.h"

void sendIRCodes();

class TvbGoneApp : public App {
public:
    const char* GetName() const override { return "TV"; }
    bool Loop() override;
    bool AutoStart() const override { return true; }
};
