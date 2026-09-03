#pragma once

#include "../App.h"
#include "../FileReader/fileReader.h"
#include "../FileViewer/fileViewer.h"
#include <vector>
#include <Arduino.h>

int getUsagePercentage();

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

    String currentPath = "/";
    std::vector<String> filesList;
    ManagerState currentState = LIST;
    std::vector<String> options;
    FileReaderApp reader;
    FileViewerApp viewer;
    int selectedIndex = 0;
    int optionIndex = 0;
};
