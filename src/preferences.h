#pragma once

#include <Arduino.h>

// Инициализация хранилища (вызывать один раз в setup)
void pref_init();

// Функции для записи (Set)
void pref_set(const char* key, int value);
void pref_set(const char* key, float value);
void pref_set(const char* key, const String& value);
void pref_set(const char* key, bool value);

// Функции для чтения (Get) с указанием значения по умолчанию
int    pref_get(const char* key, int defaultValue);
float  pref_get(const char* key, float defaultValue);
String pref_get(const char* key, const String& defaultValue);
bool   pref_get(const char* key, bool defaultValue);
