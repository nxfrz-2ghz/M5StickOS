#include "M5StickCPlus2.h"
#include <LittleFS.h>
#include <vector>
#include <cstdio>
#include "../../libs/gui/gui.h"
#include "../FileReader/fileReader.h"
#include "../FileViewer/fileViewer.h"
#include "fileManager.h"

bool FileManagerApp::isImageFile(const String &filename) const {
    String name = filename;
    name.toLowerCase();
    return name.endsWith(".bmp");
}

bool FileManagerApp::isTextFile(const String &filename) const {
    String name = filename;
    name.toLowerCase();
    return name.endsWith(".txt") || name.endsWith(".log") || name.endsWith(".csv") ||
           name.endsWith(".md") || name.endsWith(".ini") || name.endsWith(".json") ||
           name.endsWith(".h") || name.endsWith(".cpp") || name.endsWith(".ino");
}

void FileManagerApp::updateFilesArray(const String &path) {
    filesList.clear();
    File root = LittleFS.open(path);
    if (!root || !root.isDirectory()) return;

    File file = root.openNextFile();
    while (file) {
        filesList.push_back(String(file.name()));
        file = root.openNextFile();
    }
}

void FileManagerApp::updateOptions() {
    options.clear();
    if (filesList.empty()) {
        options = {"Delete", "Close"};
        return;
    }

    const String &filename = filesList[selectedIndex];
    if (isImageFile(filename)) {
        options = {"View", "Delete", "Close"};
    } else {
        options = {"Open", "Delete", "Close"};
    }
}

void FileManagerApp::displayUI() {
    if (currentState == LIST) {
        displayList("--- Files ---", filesList, selectedIndex);
    } else {
        updateOptions();
        displayList("Actions:", options, optionIndex);
    }
}

void FileManagerApp::handleAction() {
    if (optionIndex == 0) {
        const String &filename = filesList[selectedIndex];
        if (isImageFile(filename)) {
            openImageFile(filename);
            currentState = IMAGE_VIEWER;
        } else {
            openTextFile(filename);
            currentState = READER;
        }
        return;
    } else if (optionIndex == 1) {
        String fullPath = currentPath + (currentPath.endsWith("/") ? "" : "/") + filesList[selectedIndex];
        if (LittleFS.remove(fullPath)) {
            displayText("Deleted!");
            updateFilesArray(currentPath);
            selectedIndex = 0;
        } else {
            displayText("Error delete");
        }
        delay(1000);
    }

    currentState = LIST;
    optionIndex = 0;
    displayUI();
}


int getUsagePercentage() {
    if (!LittleFS.begin(true)) {
        return -1;
    }
    size_t total = LittleFS.totalBytes();
    size_t used = LittleFS.usedBytes();
    if (total == 0) return -1;
    return (used * 100) / total;
}

const char* FileManagerApp::StartPrompt() {
    // StartPrompt() — static и вызывается каждый кадр, пока висит экран
    // "нажмите A", поэтому обычный String означал бы аллокацию на каждый
    // кадр. Буфер — static-локальная переменная функции: живёт между
    // вызовами точно так же, как раньше жило mutable-поле объекта.
    static char buf[24];
    snprintf(buf, sizeof(buf), "FULL: %d%%", getUsagePercentage());
    return buf;
}

void FileManagerApp::Setup() {
    if (!LittleFS.begin(true)) {
        displayText("FS Mount Failed");
        return;
    }
    updateFilesArray(currentPath);
    displayUI();
}

bool FileManagerApp::Loop() {
    if (currentState == READER) {
        if (!loopFileReader()) {
            currentState = LIST;
            displayUI();
        }
        return true;
    }

    if (currentState == IMAGE_VIEWER) {
        if (!loopImageViewer()) {
            currentState = LIST;
            displayUI();
        }
        return true;
    }

    if (currentState == LIST) {
        if (StickCP2.BtnPWR.wasPressed() && (filesList.empty() || selectedIndex == 0)) {
            return false;
        }

        if (!filesList.empty()) {
            int lastIndex = selectedIndex;
            selectedIndex = updateMenuSelection(selectedIndex, filesList.size());

            if (lastIndex != selectedIndex) {
                displayUI();
            }
        }
    } else if (currentState == FILE_OPTIONS) {
        int lastIndex = optionIndex;
        optionIndex = updateMenuSelection(optionIndex, options.size());

        if (lastIndex != optionIndex) {
            displayUI();
        }
    }

    if (StickCP2.BtnA.wasPressed()) {
        if (currentState == LIST && !filesList.empty()) {
            currentState = FILE_OPTIONS;
            optionIndex = 0;
        } else if (currentState == FILE_OPTIONS) {
            handleAction();
            return true;
        }
        displayUI();
    }

    return true;
}
