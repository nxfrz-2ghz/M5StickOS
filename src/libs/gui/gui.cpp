#include "gui.h"
#include <LittleFS.h>
#include "M5StickCPlus2.h"
#include "gui.h"

static const char* kCustomFontPath = "/system/font.vlw";

namespace {
    int wrappedLineCount(const String& text) {
        if (text.length() == 0) {
            return 1;
        }
        return (text.length() + GUI::listCharsPerLine - 1) / GUI::listCharsPerLine;
    }
}

void GUI::clear() {
    StickCP2.Display.fillRect(0, 0, StickCP2.Display.width(), StickCP2.Display.height(), BLACK);
}

void GUI::bigText(const char* text) {
    StickCP2.Display.startWrite();
    StickCP2.Display.setFont(&fonts::FreeSansBold18pt7b);
    StickCP2.Display.setTextDatum(middle_center);
    StickCP2.Display.drawString(text, StickCP2.Display.width() / 2, StickCP2.Display.height() / 2);
    StickCP2.Display.endWrite();
}

void GUI::text(const char* text) {
    StickCP2.Display.startWrite();
    StickCP2.Display.fillScreen(BLACK);

    bool customLoaded = StickCP2.Display.loadFont(LittleFS, kCustomFontPath);
    if (!customLoaded) StickCP2.Display.setFont(&fonts::FreeSansBold12pt7b);

    StickCP2.Display.setTextSize(2);
    StickCP2.Display.setTextColor(WHITE, BLACK);
    StickCP2.Display.setCursor(0, 10);
    StickCP2.Display.print(text);
    StickCP2.Display.setTextSize(1);
    StickCP2.Display.endWrite();
}

void GUI::line(const char* text, int lineNumber) {
    StickCP2.Display.startWrite();

    bool customLoaded = StickCP2.Display.loadFont(LittleFS, kCustomFontPath);
    if (!customLoaded) StickCP2.Display.setFont(&fonts::FreeSansBold12pt7b);

    StickCP2.Display.setTextSize(2);
    StickCP2.Display.setTextColor(WHITE, BLACK);

    int currentFontHeight = StickCP2.Display.fontHeight(); 
    int yPos = lineNumber * currentFontHeight;

    StickCP2.Display.fillRect(0, yPos, StickCP2.Display.width(), currentFontHeight, BLACK);

    StickCP2.Display.setCursor(5, 10 + yPos);
    StickCP2.Display.print(text);

    StickCP2.Display.setTextSize(1);
    StickCP2.Display.endWrite();
}

void GUI::list(const char* title, const std::vector<String> &list, int selectedIndex, bool enableWrap) {
    StickCP2.Display.startWrite();
    StickCP2.Display.fillScreen(BLACK);

    // TITLE
    StickCP2.Display.setFont(&fonts::FreeSansBold12pt7b);
    StickCP2.Display.setTextColor(WHITE);
    StickCP2.Display.setCursor(5, 14);
    StickCP2.Display.print(title);

    // TEXT
    StickCP2.Display.setFont(&fonts::FreeSans9pt7b);
  
    int startY = 35; 
    const int lineHeight = 20; 
    int printedLinesCount = 0;
    int firstItem = selectedIndex; 

    for (int itemIndex = firstItem; itemIndex < static_cast<int>(list.size()); itemIndex++) {
        if (printedLinesCount >= listLinesPerPage) {
            break;
        }

        const String& item = list[itemIndex];
        bool isSelected = (itemIndex == selectedIndex);
        String prefix = isSelected ? "> " : "  ";

        if (enableWrap) {
            int pos = 0;
            bool isFirstLineOfItem = true;

            while (pos < static_cast<int>(item.length()) && printedLinesCount < listLinesPerPage) {
                int currentPrefixLen = isFirstLineOfItem ? prefix.length() : 2;
                int maxChars = max(1, listCharsPerLine - currentPrefixLen);
                
                String chunk = item.substring(pos, pos + maxChars);
                String displayLineStr = (isFirstLineOfItem ? prefix : "  ") + chunk;

                StickCP2.Display.setCursor(5, startY + (printedLinesCount * lineHeight));
                StickCP2.Display.print(displayLineStr);

                printedLinesCount++;
                pos += maxChars;
                isFirstLineOfItem = false;
            }
        } else {
            int maxChars = max(0, listCharsPerLine - static_cast<int>(prefix.length()));
            String displayLineStr = prefix + item.substring(0, maxChars);

            StickCP2.Display.setCursor(5, startY + (printedLinesCount * lineHeight));
            StickCP2.Display.print(displayLineStr);

            printedLinesCount++;
        }
    }

    StickCP2.Display.endWrite();
}

void GUI::progressBar(float progress, int barHeight) {
    int totalWidth = StickCP2.Display.width();
    int barWidth = (int)(totalWidth * progress);
    int barY = StickCP2.Display.height() - barHeight;

    StickCP2.Display.fillRect(barWidth, barY, totalWidth - barWidth, barHeight, BLACK);

    uint16_t barColor = (progress > 0.6) ? TFT_GREEN : (progress > 0.3 ? TFT_YELLOW : TFT_RED);

    StickCP2.Display.fillRect(0, barY, barWidth, barHeight, barColor);
}

int GUI::GUI::updateMenuSelection(int index, const int max, const int updateDelay) {
    if (max <= 0) {
        return 0;
    }
  
    const unsigned long initialDelay = 500;
    const unsigned long minDelay = 50;
    const unsigned long accelerationSteps = 30;

    static unsigned long btnPressedTime = 0;   
    static unsigned long lastStepTime = 0;      
    unsigned long currentTime = millis();
  
    static byte lastPressedBtn = 0;             
    byte currentBtn = 0;
  
    if (StickCP2.BtnB.isPressed()) currentBtn = 1;
    else if (StickCP2.BtnPWR.isPressed()) currentBtn = 2;

    if (currentBtn != 0 && lastPressedBtn == 0) {
        lastPressedBtn = currentBtn;
        btnPressedTime = currentTime;
        lastStepTime = currentTime;

        if (currentBtn == 1 && index < max - 1) { index++; clear(); }
        else if (currentBtn == 2 && index > 0) { index--; clear(); }
        return index;
    }

    if (currentBtn == 0 || currentBtn != lastPressedBtn) {
        lastPressedBtn = currentBtn; 
        return index;
    }

    if (currentTime - btnPressedTime < initialDelay) {
        return index; 
    }

    unsigned long holdDuration = currentTime - btnPressedTime - initialDelay; 
  
    long dynamicDelay = (long)updateDelay - (long)(holdDuration / accelerationSteps);
    if (dynamicDelay < (long)minDelay) {
        dynamicDelay = minDelay;
    }

    if (currentTime - lastStepTime >= (unsigned long)dynamicDelay) {
        lastStepTime = currentTime;

        if (currentBtn == 1 && index < max - 1) { index++; clear(); }
        else if (currentBtn == 2 && index > 0) { index--; clear(); }
    }

    return index;
}
