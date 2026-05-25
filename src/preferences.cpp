#include <Arduino.h>
#include "preferences.h"

// Simple implementation using LittleFS instead of Preferences
#include <LittleFS.h>

// For now, provide stub implementations to allow compilation
// The actual Preferences functionality can be implemented with LittleFS

void pref_init() {
    Serial.println("[Pref] Storage initialized (stub)");
}

void pref_set(const char* key, int value) {
    // TODO: Implement with LittleFS
}

void pref_set(const char* key, float value) {
    // TODO: Implement with LittleFS
}

void pref_set(const char* key, const String& value) {
    // TODO: Implement with LittleFS
}

void pref_set(const char* key, bool value) {
    // TODO: Implement with LittleFS
}

int pref_get(const char* key, int defaultValue) {
    return defaultValue;
}

float pref_get(const char* key, float defaultValue) {
    return defaultValue;
}

String pref_get(const char* key, const String& defaultValue) {
    return defaultValue;
}

bool pref_get(const char* key, bool defaultValue) {
    return defaultValue;
}
