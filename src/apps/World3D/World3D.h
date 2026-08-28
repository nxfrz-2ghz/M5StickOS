#pragma once

#include "../App.h"

class World3DApp : public App {
public:
    const char* GetName() const override { return "3D"; }
    void Setup() override;
    bool Loop() override;
    void Exit() override;
    const char* StartPrompt() const override { return "< RUN >"; }
};

extern World3DApp world3DApp;
