#include "prefs.h"
#include <Preferences.h>

Preferences prefs;

bool prefGetBool(const String& key, bool defaultValue) {
    prefs.begin("", true);
    bool value = prefs.getBool(key.c_str(), defaultValue);
    prefs.end();
    return value;
}

int prefGetInt(const String& key, int defaultValue) {
    prefs.begin("", true);
    int value = prefs.getInt(key.c_str(), defaultValue);
    prefs.end();
    return value;
}

String prefGetString(const String& key, const String& defaultValue) {
    prefs.begin("", true);
    String value = prefs.getString(key.c_str(), defaultValue);
    prefs.end();
    return value;
}

void prefSetBool(const String& key, bool value) {
    prefs.begin("");
    prefs.putBool(key.c_str(), value);
    prefs.end();
}

void prefSetInt(const String& key, int value) {
    prefs.begin("");
    prefs.putInt(key.c_str(), value);
    prefs.end();
}

void prefSetString(const String& key, const String& value) {
    prefs.begin("");
    prefs.putString(key.c_str(), value);
    prefs.end();
}
