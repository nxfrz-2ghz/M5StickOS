#pragma once

#include "../App.h"
#include "worldIrCodes.h"

void delay_ten_us(uint16_t us);
void sendIRCodes();

class TvbGoneApp : public App {
public:
    const char* GetName() const override { return "TV"; }
    bool Loop() override;
    bool AutoStart() const override { return true; }
};

extern TvbGoneApp tvbGoneApp;
