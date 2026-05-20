#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "M5StickCPlus2.h"
#include <LittleFS.h>
#include <vector>
#include "gui.h"
#include "btnC.h"
#include "file_reader.h"

void initFileManager();
void loopFileManager();

#endif