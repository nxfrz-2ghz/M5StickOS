#pragma once

#include "../App.h"

class WebServerApp : public App {
public:
    const char* GetName() const override { return "Web"; }
    void Setup() override;
    bool Loop() override;
};

extern WebServerApp webServerApp;
