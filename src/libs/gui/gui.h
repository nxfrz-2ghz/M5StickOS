#pragma once

#include <vector>
#include <Arduino.h>

const int linesPerPage = 5;
const int charsPerLine = 18;

// Принимают const char*, а не String: эти функции дёргаются каждый кадр
// (экран ожидания запуска, часы на рабочем столе, TV-B-Gone), и String
// на каждый такой вызов означал бы аллокацию/освобождение на куче 20+
// раз в секунду. Для динамического текста собирайте его в свой char-буфер
// (snprintf) и передавайте buf, либо, если строка уже есть как String,
// вызовите str.c_str().
void displayBigText(const char* text);
void displayText(const char* text);

// Список файлов/пунктов меню строится не каждый кадр, а только при смене
// выбора, поэтому здесь оставлен std::vector<String> (имена файлов и так
// приходят как String из LittleFS, а их длина заранее не ограничена).
// Передаём по константной ссылке, чтобы не копировать вектор на каждый вызов.
void displayList(const char* title, const std::vector<String> &list, int selectedIndex = -1);

int updateMenuSelection(int index, const int max);
int updateMenuSelectionFast(int index, const int max);
