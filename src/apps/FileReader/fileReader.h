#pragma once

#include "../App.h"
#include <vector>

class FileReaderApp : public App {
public:
    FileReaderApp() = default;
    explicit FileReaderApp(const String& filename) : filename(filename) {}

    static const char* GetName() { return "Reader"; }

    void SetFile(const String& filename);
    void Setup() override;
    bool Loop() override;
    void Exit() override;

private:
    void splitLines(const String& text);
    void renderPage();

    String filename;
    String fileContent;
    std::vector<String> lines;
    int currentLine = 0;
    int maxScroll = 0;
    bool opened = false;
};
