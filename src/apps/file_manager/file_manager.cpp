#include "file_manager.h"

#include <LittleFS.h>
#include <vector>
#include <cstdio>

#include "../../libs/gui/gui.h"

static const char* kUpEntryName = "..";

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

String FileManagerApp::joinPath(const String &base, const String &name) const {
    if (base == "/") return "/" + name;
    return base + "/" + name;
}

String FileManagerApp::parentPath(const String &path) const {
    if (path == "/" || path.length() == 0) return "/";

    String p = path;
    if (p.endsWith("/")) p.remove(p.length() - 1);

    int idx = p.lastIndexOf('/');
    if (idx <= 0) return "/";
    return p.substring(0, idx);
}

bool FileManagerApp::removeRecursive(const String &path) const {
    File entry = LittleFS.open(path);
    if (!entry) return false;

    if (!entry.isDirectory()) {
        entry.close();
        return LittleFS.remove(path);
    }

    std::vector<String> children;
    File child = entry.openNextFile();
    while (child) {
        children.push_back(String(child.name()));
        child = entry.openNextFile();
    }
    entry.close();

    bool ok = true;
    for (const auto &childName : children) {
        String childPath = joinPath(path, childName);
        ok = removeRecursive(childPath) && ok;
    }
    return LittleFS.rmdir(path) && ok;
}

void FileManagerApp::updateFilesArray(const String &path) {
    filesList.clear();

    if (path != "/") {
        filesList.push_back({kUpEntryName, true});
    }

    File root = LittleFS.open(path);
    if (!root || !root.isDirectory()) return;

    File file = root.openNextFile();
    while (file) {
        filesList.push_back({String(file.name()), file.isDirectory()});
        file = root.openNextFile();
    }
    root.close();
}

void FileManagerApp::updateOptions() {
    options.clear();
    if (filesList.empty()) {
        options = {"Close"};
        return;
    }

    const FileEntry &entry = filesList[selectedIndex];

    if (entry.isDir) {
        options = {"Open", "Delete", "Close"};
    } else if (isImageFile(entry.name)) {
        options = {"View", "Delete", "Close"};
    } else {
        options = {"Open", "Delete", "Close"};
    }
}

void FileManagerApp::displayUI() {
    if (currentState == LIST) {
        std::vector<String> displayNames;
        displayNames.reserve(filesList.size());

        for (const auto &entry : filesList) {
            if (entry.name == kUpEntryName) {
                displayNames.push_back("[..]");
            } else if (entry.isDir) {
                displayNames.push_back(entry.name + "/");
            } else {
                displayNames.push_back(entry.name);
            }
        }

        displayList("--- Files ---", displayNames, selectedIndex);
    } else {
        updateOptions();
        displayList("Actions:", options, optionIndex);
    }
}

void FileManagerApp::handleAction() {
    const FileEntry entry = filesList[selectedIndex];

    if (optionIndex == 0) {
        if (entry.isDir) {
            currentPath = joinPath(currentPath, entry.name);
            updateFilesArray(currentPath);
            selectedIndex = 0;
            currentState = LIST;
            displayUI();
            return;
        }

        String fullPath = joinPath(currentPath, entry.name);
        if (isImageFile(entry.name)) {
            viewer.SetFile(fullPath);
            viewer.Setup();
            currentState = IMAGE_VIEWER;
        } else {
            reader.SetFile(fullPath);
            reader.Setup();
            currentState = READER;
        }
        return;
    } else if (optionIndex == 1 && entry.name != kUpEntryName) {
        // Delete (для файла или папки со всем содержимым)
        String fullPath = joinPath(currentPath, entry.name);
        bool ok = entry.isDir ? removeRecursive(fullPath) : LittleFS.remove(fullPath);

        if (ok) {
            displayText(entry.isDir ? "Folder deleted!" : "Deleted!");
            updateFilesArray(currentPath);
            if (selectedIndex >= static_cast<int>(filesList.size())) {
                selectedIndex = filesList.empty() ? 0 : static_cast<int>(filesList.size()) - 1;
            }
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
    static char buf[24];
    snprintf(buf, sizeof(buf), "FULL: %d%%", getUsagePercentage());
    return buf;
}

void FileManagerApp::Setup() {
    if (!LittleFS.begin(true)) {
        displayText("FS Mount Failed");
        return;
    }
    currentPath = "/";
    selectedIndex = 0;
    updateFilesArray(currentPath);
    displayUI();
}

bool FileManagerApp::Loop() {
    if (currentState == READER) {
        if (!reader.Loop()) {
            reader.Exit();
            currentState = LIST;
            displayUI();
        }
        return true;
    }

    if (currentState == IMAGE_VIEWER) {
        if (!viewer.Loop()) {
            viewer.Exit();
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
        if (filesList[selectedIndex].name == kUpEntryName) {
            currentPath = parentPath(currentPath);

            updateFilesArray(currentPath);

            selectedIndex = 0;
            currentState = LIST;

            displayUI();
            return true;
        }
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
