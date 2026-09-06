#pragma once

#include "../app.h"
#include "../web_server/server_backend_task.h"
#include "../../core/preferences/prefs.h"

class SettingsApp : public App {
public:
    static const char* GetName() { return "Settings"; }

    void Setup() override;
    bool Loop() override;
private:
    void renderPage();

    struct ConfigValue {
        enum Type { TYPE_BOOL, TYPE_INT, TYPE_STRING } type;
        bool bVal = false;
        int iVal = 0;
        String sVal = "";

        ConfigValue() : type(TYPE_BOOL), bVal(false) {}
        ConfigValue(bool value) : type(TYPE_BOOL), bVal(value) {}
        ConfigValue(int value) : type(TYPE_INT), iVal(value) {}
        ConfigValue(const String& value) : type(TYPE_STRING), sVal(value) {}
        ConfigValue(const char* value) : type(TYPE_STRING), sVal(value ? String(value) : String()) {}
    };

    static constexpr std::string_view settingKeys[] = {
        "Brightness", "NETWORK", "STA SSID", "STA PASS",
        "WEB_SERVER", "AP SSID", "AP PASS", "IS_AP_OR_STA"
    };

    std::vector<ConfigValue> settingValues = {
        ConfigValue(prefGetInt("brightness", 100)),
        ConfigValue(""),
        ConfigValue(prefGetString(KEY_STA_SSID, DEFAULT_STA_SSID)),
        ConfigValue(prefGetString(KEY_STA_PASS, DEFAULT_STA_PASS)),
        ConfigValue(""),
        ConfigValue(prefGetString(KEY_AP_SSID, DEFAULT_AP_SSID)),
        ConfigValue(prefGetString(KEY_AP_PASS, DEFAULT_AP_PASS)),
        ConfigValue(prefGetInt(KEY_SERVER_MODE, DEFAULT_MODE))
    };

    static constexpr byte settingsCount = sizeof(settingKeys) / sizeof(settingKeys[0]);
    byte selectedIndex = 0;
};
