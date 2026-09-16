#include "file_reader.h"

#include <LittleFS.h>
#include "../../libs/gui/gui.h"

namespace {
int utf8CharLen(unsigned char leadByte) {
    if ((leadByte & 0x80) == 0x00) return 1;  // 0xxxxxxx — ASCII
    if ((leadByte & 0xE0) == 0xC0) return 2;  // 110xxxxx — напр. кириллица
    if ((leadByte & 0xF0) == 0xE0) return 3;  // 1110xxxx
    if ((leadByte & 0xF8) == 0xF0) return 4;  // 11110xxx
    return 1;  // некорректный байт — считаем как одиночный, чтобы не зависнуть
}
}

void FileReaderApp::SetFile(const String& value) {
    filename = value;
}

void FileReaderApp::splitLines(const String& text) {
    lines.clear();
    String temp;
    int charsInLine = 0;

    int i = 0;
    int len = text.length();

    while (i < len) {
        unsigned char c = static_cast<unsigned char>(text[i]);

        if (c == '\n') {
            lines.push_back(temp);
            temp = "";
            charsInLine = 0;
            i++;
            continue;
        }

        int charLen = utf8CharLen(c);
        if (i + charLen > len) charLen = len - i;  // защита от обрезанного файла

        temp += text.substring(i, i + charLen);
        charsInLine++;
        i += charLen;

        if (charsInLine >= displayTextCharsPerLine) {
            lines.push_back(temp);
            temp = "";
            charsInLine = 0;
        }
    }

    if (temp.length() > 0) {
        lines.push_back(temp);
    }

    maxScroll = static_cast<int>(lines.size()) - displayTextLinesPerPage;
    if (maxScroll < 0) {
        maxScroll = 0;
    }
}

void FileReaderApp::renderPage() {
    String output;
    int endLine = currentLine + displayTextLinesPerPage;

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
    currentLine = updateMenuSelection(currentLine, maxScroll + 1);

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
