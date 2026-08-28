#pragma once

#include "../App.h"

int getUsagePercentage();

class FileManagerApp : public App {
public:
    const char* GetName() const override { return "Files"; }
    void Setup() override;
    bool Loop() override;
    const char* StartPrompt() const override;

private:
    // Заполняется в StartPrompt() через snprintf — вызывается каждый кадр,
    // пока висит экран "нажмите A", поэтому обычный String означал бы
    // аллокацию на каждый кадр. Метод StartPrompt() отмечен const, поэтому
    // буфер должен быть mutable, чтобы можно было обновлять его без
    // изменения логического состояния объекта.
    mutable char startPromptBuf[24] = {0};
};

extern FileManagerApp fileManagerApp;
