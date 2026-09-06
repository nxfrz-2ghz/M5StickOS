#pragma once

#include "../app.h"
#include <LittleFS.h>

class FileViewerApp : public App {
public:
    FileViewerApp() = default;
    explicit FileViewerApp(const String& filename) : filename(filename) {}

    static const char* GetName() { return "Viewer"; }

    void SetFile(const String& filename);
    void Setup() override;
    bool Loop() override;
    void Exit() override;

private:
    static uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b);
    static uint16_t read16(File& file);
    static uint32_t read32(File& file);

    bool getBmpInfo(File& file, int32_t& width, int32_t& height, uint16_t& bitCount);
    bool drawBmp(File& file);
    bool loadAndRenderBmp(File& file);

    String filename;
    int originalRotation = 0;
    bool opened = false;
};
