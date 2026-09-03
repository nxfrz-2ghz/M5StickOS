#include <LittleFS.h>
#include "fileReader.h"
#include "../../libs/gui/gui.h"

void FileReaderApp::SetFile(const String& value) {
    filename = value;
}

void FileReaderApp::splitLines(const String& text) {
    lines.clear();
    String temp;

    for (int i = 0; i < text.length(); ++i) {
        char c = text[i];

        if (c == '\n') {
            lines.push_back(temp);
            temp = "";
            continue;
        }

        temp += c;

        if (temp.length() >= charsPerLine) {
            lines.push_back(temp);
            temp = "";
        }
    }

    if (temp.length() > 0) {
        lines.push_back(temp);
    }

    maxScroll = static_cast<int>(lines.size()) - linesPerPage;
    if (maxScroll < 0) {
        maxScroll = 0;
    }
}

void FileReaderApp::renderPage() {
    String output;
    int endLine = currentLine + linesPerPage;

    if (endLine > static_cast<int>(lines.size())) {
        endLine = lines.size();
    }

    for (int i = currentLine; i < endLine; i++) {
        output += lines[i];
        output += '\n';
    }

    displayText(output.c_str());
}

void FileReaderApp::Setup() {
    opened = false;
    fileContent = "";
    lines.clear();
    currentLine = 0;
    maxScroll = 0;

    String path = filename.startsWith("/") ? filename : "/" + filename;
    File file = LittleFS.open(path, FILE_READ);

    if (!file) {
        displayText("Error:\nCan't open file");
        delay(1500);
        return;
    }

    while (file.available()) {
        fileContent += static_cast<char>(file.read());
    }

    file.close();

    splitLines(fileContent);
    opened = true;
    renderPage();
}

bool FileReaderApp::Loop() {
    if (!opened || StickCP2.BtnA.wasPressed()) {
        return false;
    }

    int oldLine = currentLine;
    currentLine = updateMenuSelectionFast(currentLine, maxScroll + 1);

    if (oldLine != currentLine) {
        renderPage();
    }

    return true;
}

void FileReaderApp::Exit() {
    fileContent = "";
    lines.clear();
    currentLine = 0;
    maxScroll = 0;
    opened = false;
}
