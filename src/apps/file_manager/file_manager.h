#pragma once

#include "../app.h"
#include "../file_reader/file_reader.h"
#include "../file_viewer/file_viewer.h"
#include <vector>
#include <Arduino.h>

int getUsagePercentage();

// Один элемент списка: файл или папка.
struct FileEntry {
    String name;
    bool isDir;
};

class FileManagerApp : public App {
public:
    static const char* GetName() { return "Files"; }
    static const char* StartPrompt();

    void Setup() override;
    bool Loop() override;

private:
    enum ManagerState { LIST, FILE_OPTIONS, READER, IMAGE_VIEWER };

    bool isImageFile(const String &filename) const;
    bool isTextFile(const String &filename) const;
    void updateFilesArray(const String &path);
    void updateOptions();
    void displayUI();
    void handleAction();

    String joinPath(const String &base, const String &name) const;
    String parentPath(const String &path) const;
    bool removeRecursive(const String &path) const;

    String currentPath = "/";
    std::vector<FileEntry> filesList;
    ManagerState currentState = LIST;
    std::vector<String> options;
    FileReaderApp reader;
    FileViewerApp viewer;
    int selectedIndex = 0;
    int optionIndex = 0;
};
