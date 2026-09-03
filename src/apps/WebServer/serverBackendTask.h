#pragma once

#include "../Task.h"
#include <WebServer.h>
#include <FS.h>

// --- Defaults ---
#define DEFAULT_AP_SSID     "M5Stick_AP"
#define DEFAULT_AP_PASS     "password123"
#define DEFAULT_STA_SSID    ""
#define DEFAULT_STA_PASS    ""
#define DEFAULT_MODE        0   // 0 = AP, 1 = STA

#define KEY_AP_SSID "ap_ssid"
#define KEY_AP_PASS "ap_pass"
#define KEY_STA_SSID "sta_ssid"
#define KEY_STA_PASS "sta_pass"
#define KEY_SERVER_MODE "mode"

class ServerBackendTask : public Task {
public:
    static const char* GetName() { return "Server"; }

    void Setup() override;
    bool Loop() override;
    void Stop() override;

    // --- Публичное API для frontend-приложения ---
    void ToggleMode();

    bool IsStaMode() const { return cfg_mode; }
    String StatusText() const { return statusText_; }

private:
    void loadConfig();
    void saveConfig();
    void startAP();
    bool startSTA();
    void stopNetwork();
    void applyMode();

    String currentDateString() const;
    void saveTextToFile(const String &filename, const String &text);
    String normalizeUploadImageFilename(const String &filename) const;

    void handleRoot();
    void handleSetText();
    void handleSetTime();
    void handleGetTime();
    void handleSaveConfig();
    void handleUploadImagePost();
    void handleUploadImage();

    WebServer server_{80};

    String cfg_ap_ssid   = DEFAULT_AP_SSID;
    String cfg_ap_pass   = DEFAULT_AP_PASS;
    String cfg_sta_ssid  = DEFAULT_STA_SSID;
    String cfg_sta_pass  = DEFAULT_STA_PASS;
    bool   cfg_mode      = DEFAULT_MODE;   // текущий активный режим

    String statusText_ = "Starting...";

    File imageUploadFile_;
    String imageUploadResponse_ = "No image uploaded";
    bool imageUploadOpen_ = false;
    String imageUploadSavedName_;
    size_t imageUploadBytes_ = 0;
    static const size_t kMaxImageUploadBytes = 2 * 1024 * 1024; // 2 MB
};
