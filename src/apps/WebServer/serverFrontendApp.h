#pragma once

#include "../App.h"

// ServerFrontendApp — "лицо" веб-сервера в лаунчере: запускает
// ServerBackendTask (если он ещё не работает), показывает его статус на
// экране и позволяет им управлять. Сам сервер при этом крутится в фоне
// через TaskManager и не завершается при выходе из этого приложения
// (BtnB просто закрывает экран — сервер продолжает отвечать на запросы).
class ServerFrontendApp : public App {
public:
    const char* GetName() const override { return "Web"; }
    void Setup() override;
    bool Loop() override;

private:
    void render();
    String lastRendered;
};
