#pragma once

#include <Arduino.h>

struct Task {
    virtual ~Task() {}

    virtual void Setup() {}
    virtual bool Loop() = 0;
    virtual void Stop() {}
    
    static const char* GetName() { return "Task"; }
};
