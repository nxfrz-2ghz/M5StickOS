#pragma once

#include "../App.h"

// TODO: ещё не реализовано полноценно (см. settings.cpp) — сейчас не
// зарегистрировано в main.cpp / apps list.
class SettingsApp : public App {
public:
    const char* GetName() const override { return "Settings"; }
    bool Loop() override { return true; }
};
