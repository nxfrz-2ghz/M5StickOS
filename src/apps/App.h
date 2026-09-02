#pragma once

#include "M5StickCPlus2.h"

struct App{
    virtual ~App() {}

    virtual void Setup() {}
    virtual bool Loop() = 0;
    virtual void Exit() {}

    static const char* GetName() { return "App"; }
    static const char* StartPrompt() { return "< START >"; }
    static bool AutoStart() { return false; }
};
