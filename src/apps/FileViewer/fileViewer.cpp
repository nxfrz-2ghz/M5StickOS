#include "M5StickCPlus2.h"
#include <LittleFS.h>
#include <vector>
#include "../../libs/gui/gui.h"
#include "fileViewer.h"

static uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static uint16_t read16(File &file) {
  uint16_t value = file.read();
  value |= (uint16_t)file.read() << 8;
  return value;
}

static uint32_t read32(File &file) {
  uint32_t value = file.read();
  value |= (uint32_t)file.read() << 8;
  value |= (uint32_t)file.read() << 16;
  value |= (uint32_t)file.read() << 24;
  return value;
}

static bool drawBmp(File &file) {
  file.seek(0);
  if (read16(file) != 0x4D42) {
    return false; // не BMP
  }

  file.seek(10);
  uint32_t pixelDataOffset = read32(file);
  file.seek(18);

  int32_t bmpWidth = (int32_t)read32(file);
  int32_t bmpHeight = (int32_t)read32(file);
  uint16_t planes = read16(file);
  uint16_t bitCount = read16(file);
  uint32_t compression = read32(file);

  if (planes != 1 || compression != 0 || bmpWidth <= 0 || bmpHeight <= 0) {
    return false;
  }

  if (bitCount != 24 && bitCount != 16 && bitCount != 32) {
    return false;
  }

  uint32_t rowSize = ((uint32_t)bitCount * bmpWidth + 31) / 32 * 4;
  std::vector<uint8_t> rowBuf(rowSize);

  int targetW = StickCP2.Display.width();
  int targetH = StickCP2.Display.height();

  float scaleX = (float)bmpWidth / targetW;
  float scaleY = (float)bmpHeight / targetH;
  float scale = max(1.0f, max(scaleX, scaleY));

  int drawW = min(targetW, (int)(bmpWidth / scale + 0.5f));
  int drawH = min(targetH, (int)(bmpHeight / scale + 0.5f));
  int offsetX = (targetW - drawW) / 2;
  int offsetY = (targetH - drawH) / 2;

  float srcXScale = (float)bmpWidth / drawW;
  float srcYScale = (float)bmpHeight / drawH;

  StickCP2.Display.startWrite();
  StickCP2.Display.fillScreen(BLACK);

  for (int sy = 0; sy < drawH; sy++) {
    int srcY = min(bmpHeight - 1, (int)(sy * srcYScale));
    srcY = bmpHeight - 1 - srcY;

    file.seek(pixelDataOffset + (uint32_t)srcY * rowSize);
    if (file.read(rowBuf.data(), rowSize) != rowSize) {
      StickCP2.Display.endWrite();
      return false;
    }

    for (int sx = 0; sx < drawW; sx++) {
      int srcX = min(bmpWidth - 1, (int)(sx * srcXScale));
      uint16_t color = 0;
      uint32_t pos = (uint32_t)srcX * (bitCount / 8);

      if (pos + (bitCount / 8) > rowSize) {
        continue;
      }

      if (bitCount == 24) {
        uint8_t b = rowBuf[pos + 0];
        uint8_t g = rowBuf[pos + 1];
        uint8_t r = rowBuf[pos + 2];
        color = rgbTo565(r, g, b);
      } else if (bitCount == 32) {
        uint8_t b = rowBuf[pos + 0];
        uint8_t g = rowBuf[pos + 1];
        uint8_t r = rowBuf[pos + 2];
        color = rgbTo565(r, g, b);
      } else if (bitCount == 16) {
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

static bool loadAndRenderBmp(File &file) {
  return drawBmp(file);
}

// Получить базовую информацию о BMP (ширина, высота, битность).
static bool getBmpInfo(File &file, int32_t &bmpWidth, int32_t &bmpHeight, uint16_t &bitCount) {
  file.seek(0);
  if (read16(file) != 0x4D42) return false;

  file.seek(18);
  bmpWidth = (int32_t)read32(file);
  bmpHeight = (int32_t)read32(file);
  uint16_t planes = read16(file);
  bitCount = read16(file);
  uint32_t compression = read32(file);

  if (planes != 1 || compression != 0 || bmpWidth <= 0 || bmpHeight <= 0) {
    return false;
  }
  return true;
}

void openImageFile(const String &filename) {
  String path = filename.startsWith("/") ? filename : "/" + filename;
  File file = LittleFS.open(path, FILE_READ);

  if (!file) {
    displayText("Error:\nCan't open file");
    delay(1500);
    return;
  }

  displayText("Loading image...");

  // Сначала получим информацию об изображении, чтобы при необходимости
  // поменять ориентацию дисплея (портрет/ландшафт).
  int32_t bmpW = 0, bmpH = 0;
  uint16_t bpp = 0;
  bool infoOk = getBmpInfo(file, bmpW, bmpH, bpp);

  // Сохраним текущую ориентацию дисплея (предполагаем: 0 = портрет, 3 = ландшафт)
  bool origLandscape = StickCP2.Display.width() > StickCP2.Display.height();
  int origRotation = origLandscape ? 3 : 0;

  if (infoOk) {
    bool imgPortrait = (abs(bmpH) > abs(bmpW));
    int newRotation = imgPortrait ? 0 : 3; // 0 — портрет, 3 — ландшафт (используются в проекте)
    StickCP2.Display.setRotation(newRotation);
  }

  // Переместимся в начало и отобразим
  file.seek(0);
  if (!loadAndRenderBmp(file)) {
    displayText("Unsupported\nimage format");
    delay(1500);
  } else {
    // Ожидаем выхода в цикле просмотра, чтобы пользователь успел увидеть изображение
    while (loopImageViewer()) {
      delay(10);
    }
  }

  // Восстановим прежнюю ориентацию дисплея
  StickCP2.Display.setRotation(origRotation);

  file.close();
}

bool loopImageViewer() {
  if (StickCP2.BtnA.wasPressed() || StickCP2.BtnPWR.wasPressed()) {
    return false;
  }
  return true;
}
