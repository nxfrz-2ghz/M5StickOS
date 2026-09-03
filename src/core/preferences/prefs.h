#pragma once

#include <Arduino.h>

bool prefGetBool(const String& key, bool defaultValue);
int prefGetInt(const String& key, int defaultValue);
String prefGetString(const String& key, const String& defaultValue);

void prefSetBool(const String& key, bool value);
void prefSetInt(const String& key, int value);
void prefSetString(const String& key, const String& value);