#include "settings.h"

#include <string>
#include <unordered_map>

#include "../../libs/preferences/prefs.h"

#include "../WebServer/serverBackendTask.h"

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

static std::unordered_map<std::string, ConfigValue> settingsList = {
    {"Brightness", prefGetInt("brightness", 100)},
    {"NETWORK", ConfigValue("")},
    {"STA SSID", prefGetString(KEY_STA_SSID, DEFAULT_STA_SSID)},
    {"STA PASS", prefGetString(KEY_STA_PASS, DEFAULT_STA_PASS)},
    {"WEB_SERVER", ConfigValue("")},
    {"AP SSID", prefGetString(KEY_AP_SSID, DEFAULT_AP_SSID)},
    {"AP PASS", prefGetString(KEY_AP_PASS, DEFAULT_AP_PASS)},
    {"IS_AP_OR_STA", prefGetInt(KEY_SERVER_MODE, DEFAULT_MODE)},
};

static byte selectedIndex = 0;

// TODO: just show settings (use gui display list and update menu selection)
