#pragma once

#include "../App.h"
#include "worldIrCodes.h"

class TvbGoneApp : public App {
public:
    static const char* GetName() { return "TV"; }
    static const char* StartPrompt() { return "< READY >"; }

    void Setup() override;
    bool Loop() override;

private:
    void displayProgress() const;

    const IrCode* const* codesList = nullptr;
    uint8_t totalCodes = 0;
    uint8_t currentIndex = 0;
    bool finished = false;
};
