#include "file_reader.h"

static String fileContent = "";
static std::vector<String> lines;

static int currentLine = 0;
static int maxScroll = 0;

static const int linesPerPage = 5;
static const int charsPerLine = 18;

void splitLines(const String &text) {
    lines.clear();
    String temp = "";

    for (int i = 0; i < text.length(); i++) {
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

    maxScroll = lines.size() - linesPerPage;
    if (maxScroll < 0) {maxScroll = 0;}
}



void renderPage() {
    String output = "";
    int endLine = currentLine + linesPerPage;

    if (endLine > lines.size()) {
        endLine = lines.size();
    }

    for (int i = currentLine; i < endLine; i++) {
        output += lines[i] + "\n";
    }

    displayText(output);
}



void openTextFile(const String &filename) {
    String path = filename.startsWith("/") ? filename : "/" + filename;
    File file = LittleFS.open(path, FILE_READ);

    if (!file) {
        displayText("Error:\nCan't open file");
        delay(1500);
        return;
    }

    fileContent = "";

    while (file.available()) {
        fileContent += (char)file.read();
    }

    file.close();

    splitLines(fileContent);

    currentLine = 0;

    renderPage();
}



bool loopFileReader() {

    // Exit
    if (StickCP2.BtnA.wasPressed()) {
        return false;
    }

    int oldLine = currentLine;

    currentLine = updateMenuSelection(currentLine, maxScroll + 1);

    if (oldLine != currentLine) {
        renderPage();
    }

    return true;
}