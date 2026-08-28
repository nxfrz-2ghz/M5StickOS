#pragma once

#include <Arduino.h>

struct App{
    virtual ~App() {}

    virtual const char* GetName() const = 0;
    virtual const char* StartPrompt() const { return "< START >"; }
    virtual bool AutoStart() const { return false; }
    
    virtual void Setup() {}
    virtual bool Loop() = 0;
    virtual void Exit() {}
};
