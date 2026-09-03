#include <vector>
#include "fileViewer.h"
#include "../../libs/gui/gui.h"

uint16_t FileViewerApp::rgbTo565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint16_t FileViewerApp::read16(File& file) {
    uint16_t value = file.read();
    value |= static_cast<uint16_t>(file.read()) << 8;
    return value;
}

uint32_t FileViewerApp::read32(File& file) {
    uint32_t value = file.read();
    value |= static_cast<uint32_t>(file.read()) << 8;
    value |= static_cast<uint32_t>(file.read()) << 16;
    value |= static_cast<uint32_t>(file.read()) << 24;
    return value;
}

bool FileViewerApp::drawBmp(File& file) {
    file.seek(0);

    if (read16(file) != 0x4D42) {
        return false;
    }

    file.seek(10);
    uint32_t pixelDataOffset = read32(file);
    file.seek(18);

    int32_t bmpWidth = static_cast<int32_t>(read32(file));
    int32_t bmpHeight = static_cast<int32_t>(read32(file));
    uint16_t planes = read16(file);
    uint16_t bitCount = read16(file);
    uint32_t compression = read32(file);

    if (planes != 1 || compression != 0 || bmpWidth <= 0 || bmpHeight <= 0) {
        return false;
    }

    if (bitCount != 24 && bitCount != 16 && bitCount != 32) {
        return false;
    }

    uint32_t rowSize = ((static_cast<uint32_t>(bitCount) * bmpWidth + 31) / 32) * 4;
    std::vector<uint8_t> rowBuf(rowSize);

    int targetW = StickCP2.Display.width();
    int targetH = StickCP2.Display.height();

    float scaleX = static_cast<float>(bmpWidth) / targetW;
    float scaleY = static_cast<float>(bmpHeight) / targetH;
    float scale = max(1.0f, max(scaleX, scaleY));

    int drawW = min(targetW, static_cast<int>(bmpWidth / scale + 0.5f));
    int drawH = min(targetH, static_cast<int>(bmpHeight / scale + 0.5f));
    int offsetX = (targetW - drawW) / 2;
    int offsetY = (targetH - drawH) / 2;

    float srcXScale = static_cast<float>(bmpWidth) / drawW;
    float srcYScale = static_cast<float>(bmpHeight) / drawH;

    StickCP2.Display.startWrite();
    StickCP2.Display.fillScreen(BLACK);

    for (int sy = 0; sy < drawH; ++sy) {
        int srcY = min(bmpHeight - 1, static_cast<int>(sy * srcYScale));
        srcY = bmpHeight - 1 - srcY;

        file.seek(pixelDataOffset + static_cast<uint32_t>(srcY) * rowSize);
        if (file.read(rowBuf.data(), rowSize) != static_cast<int>(rowSize)) {
            StickCP2.Display.endWrite();
            return false;
        }

        for (int sx = 0; sx < drawW; ++sx) {
            int srcX = min(bmpWidth - 1, static_cast<int>(sx * srcXScale));
            uint32_t pos = static_cast<uint32_t>(srcX) * (bitCount / 8);

            if (pos + (bitCount / 8) > rowSize) {
                continue;
            }

            uint16_t color = 0;

            if (bitCount == 24 || bitCount == 32) {
                uint8_t b = rowBuf[pos + 0];
                uint8_t g = rowBuf[pos + 1];
                uint8_t r = rowBuf[pos + 2];
                color = rgbTo565(r, g, b);
            } else {
                uint16_t pixel = rowBuf[pos] | (rowBuf[pos + 1] << 8);
                uint8_t r = (pixel >> 11) & 0x1F;
                uint8_t g = (pixel >> 5) & 0x3F;
                uint8_t b = pixel & 0x1F;
                color = rgbTo565(r << 3, g << 2, b << 3);
            }

            StickCP2.Display.drawPixel(offsetX + sx, offsetY + sy, color);
        }
    }

    StickCP2.Display.endWrite();
    return true;
}

bool FileViewerApp::loadAndRenderBmp(File& file) {
    return drawBmp(file);
}

bool FileViewerApp::getBmpInfo(
    File& file,
    int32_t& bmpWidth,
    int32_t& bmpHeight,
    uint16_t& bitCount
) {
    file.seek(0);

    if (read16(file) != 0x4D42) {
        return false;
    }

    file.seek(18);
    bmpWidth = static_cast<int32_t>(read32(file));
    bmpHeight = static_cast<int32_t>(read32(file));
    uint16_t planes = read16(file);
    bitCount = read16(file);
    uint32_t compression = read32(file);

    return planes == 1 &&
           compression == 0 &&
           bmpWidth > 0 &&
           bmpHeight > 0 &&
           (bitCount == 16 || bitCount == 24 || bitCount == 32);
}

void FileViewerApp::SetFile(const String& value) {
    filename = value;
}

void FileViewerApp::Setup() {
    opened = false;
    originalRotation = StickCP2.Display.getRotation();

    String path = filename.startsWith("/") ? filename : "/" + filename;
    File imageFile = LittleFS.open(path, FILE_READ);

    if (!imageFile) {
        displayText("Error:\nCan't open file");
        delay(1500);
        return;
    }

    displayText("Loading image...");

    int32_t bmpW = 0;
    int32_t bmpH = 0;
    uint16_t bpp = 0;

    if (getBmpInfo(imageFile, bmpW, bmpH, bpp)) {
        bool imgPortrait = abs(bmpH) > abs(bmpW);
        StickCP2.Display.setRotation(imgPortrait ? 0 : 3);
    }

    imageFile.seek(0);

    if (!loadAndRenderBmp(imageFile)) {
        displayText("Unsupported\nimage format");
        delay(1500);
        imageFile.close();
        StickCP2.Display.setRotation(originalRotation);
        return;
    }

    imageFile.close();
    opened = true;
}

bool FileViewerApp::Loop() {
    if (!opened) {
        return false;
    }

    if (StickCP2.BtnA.wasPressed() || StickCP2.BtnPWR.wasPressed()) {
        return false;
    }

    return true;
}

void FileViewerApp::Exit() {
    if (opened) {
        StickCP2.Display.setRotation(originalRotation);
    }

    opened = false;
}
