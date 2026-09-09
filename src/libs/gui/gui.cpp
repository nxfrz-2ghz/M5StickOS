#include "M5StickCPlus2.h"
#include "gui.h"


void displayClear() {
  StickCP2.Display.fillRect(0, 0, StickCP2.Display.width(), StickCP2.Display.height(), BLACK);
}


void displayBigText(const char* text) {
  StickCP2.Display.startWrite();
  StickCP2.Display.setFont(&fonts::FreeSansBold18pt7b);
  StickCP2.Display.setTextDatum(middle_center);
  StickCP2.Display.drawString(text, StickCP2.Display.width() / 2, StickCP2.Display.height() / 2);
  StickCP2.Display.endWrite();
}


void displayText(const char* text) {
    StickCP2.Display.startWrite();
    StickCP2.Display.fillScreen(BLACK);
    StickCP2.Display.setFont(&fonts::FreeSansBold12pt7b);
    StickCP2.Display.setCursor(5, 10);
    StickCP2.Display.print(text);
    StickCP2.Display.endWrite();
}


static int wrappedLineCount(const String& text) {
  if (text.length() == 0) {
    return 1;
  }

  return (text.length() + listCharsPerLine - 1) / listCharsPerLine;
}


void displayList(const char* title, const std::vector<String> &list, int selectedIndex, bool enableWrap) {
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
        String displayLine = (isFirstLineOfItem ? prefix : "  ") + chunk;

        StickCP2.Display.setCursor(5, startY + (printedLinesCount * lineHeight));
        StickCP2.Display.print(displayLine);

        printedLinesCount++;
        pos += maxChars;
        isFirstLineOfItem = false;
      }
    } else {
      int maxChars = max(0, listCharsPerLine - static_cast<int>(prefix.length()));
      String displayLine = prefix + item.substring(0, maxChars);

      StickCP2.Display.setCursor(5, startY + (printedLinesCount * lineHeight));
      StickCP2.Display.print(displayLine);

      printedLinesCount++;
    }
  }

  StickCP2.Display.endWrite();
}


void displayProgressBar(float progress, int barHeight) {
  int totalWidth = StickCP2.Display.width();
  int barWidth = (int)(totalWidth * progress);
  int barY = StickCP2.Display.height() - barHeight;

  StickCP2.Display.fillRect(barWidth, barY, totalWidth - barWidth, barHeight, BLACK);

  uint16_t barColor = (progress > 0.6) ? TFT_GREEN : (progress > 0.3 ? TFT_YELLOW : TFT_RED);

  StickCP2.Display.fillRect(0, barY, barWidth, barHeight, barColor);
}

// Menu navigation: change selected index
int updateMenuSelection(int index, const int max, const int updateDelay) {
  if (max <= 0) {
    return 0;
  }
  
  const unsigned long initialDelay = 500;
  const unsigned long minDelay = 50;
  const unsigned long accelerationSteps = 30;

  static unsigned long btnPressedTime = 0;   // Время, когда кнопку начали удерживать
  static unsigned long lastStepTime = 0;      // Время последнего шага
  unsigned long currentTime = millis();
  
  static byte lastPressedBtn = 0;             // 0 - никто, 1 - BtnB, 2 - BtnPWR
  byte currentBtn = 0;
  
  if (StickCP2.BtnB.isPressed()) currentBtn = 1;
  else if (StickCP2.BtnPWR.isPressed()) currentBtn = 2;

  // 1. Кнопку только что нажали (первое нажатие)
  if (currentBtn != 0 && lastPressedBtn == 0) {
    lastPressedBtn = currentBtn;
    btnPressedTime = currentTime;
    lastStepTime = currentTime;

    // Сразу делаем первый шаг
    if (currentBtn == 1 && index < max - 1) { index++; displayClear(); }
    else if (currentBtn == 2 && index > 0) { index--; displayClear(); }
    return index;
  }

  // 2. Кнопку отпустили или сменили на другую
  if (currentBtn == 0 || currentBtn != lastPressedBtn) {
    lastPressedBtn = currentBtn; 
    return index;
  }

  // 3. Кнопка удерживается (currentBtn == lastPressedBtn)
  // Проверяем, прошла ли стартовая задержка в 500 мс
  if (currentTime - btnPressedTime < initialDelay) {
    return index; // Еще ждем, ничего не делаем
  }

  // Здесь мы уже гарантированно в режиме автоповтора (замена флагу isHolding)
  unsigned long holdDuration = currentTime - btnPressedTime - initialDelay; 
  
  // Расчет динамической задержки
  long dynamicDelay = (long)updateDelay - (long)(holdDuration / accelerationSteps);
  if (dynamicDelay < (long)minDelay) {
    dynamicDelay = minDelay;
  }

  // Проверяем таймер автоповтора
  if (currentTime - lastStepTime >= (unsigned long)dynamicDelay) {
    lastStepTime = currentTime;

    if (currentBtn == 1 && index < max - 1) { index++; displayClear(); }
    else if (currentBtn == 2 && index > 0) { index--; displayClear(); }
  }

  return index;
}
