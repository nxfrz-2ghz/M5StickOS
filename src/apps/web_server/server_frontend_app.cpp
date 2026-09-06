#include "server_frontend_app.h"
#include "server_backend_task.h"
#include "../../core/task_manager/task_manager.h"
#include "../../libs/gui/gui.h"

// Регистрируем фабрику задачи один раз при первом обращении к этому файлу.
// TaskManager сам создаст экземпляр ServerBackendTask при первом Start().
namespace {
struct ServerTaskRegistration {
    ServerTaskRegistration() {
        TaskManager::Instance().Register("Server", []() -> Task* { return new ServerBackendTask(); });
    }
} serverTaskRegistration;
}

void ServerFrontendApp::Setup() {
    // Идемпотентно: если сервер уже работает в фоне (был запущен в
    // прошлый раз и не останавливался), Start() просто вернёт его —
    // повторного Setup() у задачи не произойдёт.
    TaskManager::Instance().Start("Server");
    lastRendered = "";
    render();
}

void ServerFrontendApp::render() {
    auto* backend = static_cast<ServerBackendTask*>(TaskManager::Instance().Find("Server"));
    String text = backend ? backend->StatusText() : "Server stopped";
    if (text != lastRendered) {
        displayText(text.c_str());
        lastRendered = text;
    }
}

bool ServerFrontendApp::Loop() {
    render();

    auto* backend = static_cast<ServerBackendTask*>(TaskManager::Instance().Find("Server"));

    if (StickCP2.BtnA.wasPressed() && backend) {
        backend->ToggleMode();
    }

    if (StickCP2.BtnB.wasPressed()) {
        // Закрываем только экран управления. Сама задача остаётся жить в
        // фоне — TaskManager::LoopAll() в main.cpp продолжит её крутить,
        // и в следующий раз это же (или другое) приложение сможет снова
        // найти её через TaskManager::Find() и подхватить статус.
        return false;
    }

    if (StickCP2.BtnPWR.wasPressed()) {
        // А это — настоящее выключение сервера, а не просто выход с экрана.
        TaskManager::Instance().Stop("Server");
        displayText("WiFi OFF");
        delay(300);
        return false;
    }

    return true;
}
