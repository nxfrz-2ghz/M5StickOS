#ifndef FILE_READER_H
#define FILE_READER_H

#include "M5StickCPlus2.h"
#include <LittleFS.h>
#include "gui.h"

void openTextFile(const String &filename);
bool loopFileReader();

#endif
