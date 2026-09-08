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
int updateMenuSelection(int index, const int max) {
  if (max <= 0) {
    return 0;
  }

  // Next
  if (StickCP2.BtnB.wasPressed()) {
    const int oldIndex = index;
    index = index + 1;
    if (index >= max) {
      index = max - 1;
    }
    else {
      displayClear();
    }
  }
  // Previous
  else if (StickCP2.BtnPWR.wasPressed()) {
    const int oldIndex = index;
    index = index - 1;
    if (index < 0) {
      index = 0;
    }
    else {
      displayClear();
    }
  }

  return index;
}

int updateMenuSelectionFast(int index, const int max, const int updateDelay) {
  if (max <= 0) {
    return 0;
  }

  // Внешние статические переменные для отслеживания состояний
  static unsigned long btnPressedTime = 0;   // Время, когда кнопку начали удерживать
  static unsigned long lastStepTime = 0;      // Время последнего шага (изменения индекса)
  static bool isHolding = false;              // Флаг, удерживается ли кнопка
  static int lastPressedBtn = 0;              // 0 - никто, 1 - BtnB, 2 - BtnPWR

  // Настройки таймингов
  const unsigned long initialDelay = 500;     // Ожидание перед стартом автоповтора
  const unsigned long minDelay = 50;          // Максимальная скорость
  const unsigned long accelerationSteps = 10; // Скорость ускорения

  // Считываем текущее состояние кнопок
  bool nextPressed = StickCP2.BtnB.isPressed();
  bool prevPressed = StickCP2.BtnPWR.isPressed();
  unsigned long currentTime = millis();

  // Определяем, какая именно кнопка активна сейчас
  int currentBtn = 0;
  if (nextPressed) currentBtn = 1;
  else if (prevPressed) currentBtn = 2;

  // ЛОГИКА 1: Кнопку только что нажали (первое нажатие)
  if (currentBtn != 0 && lastPressedBtn == 0) {
    lastPressedBtn = currentBtn;
    btnPressedTime = currentTime;
    lastStepTime = currentTime;
    isHolding = false;

    // Сразу делаем первый шаг
    if (currentBtn == 1 && index < max - 1) { index++; displayClear(); }
    else if (currentBtn == 2 && index > 0) { index--; displayClear(); }
    return index;
  }

  // ЛОГИКА 2: Кнопку отпустили или сменили на другую
  if (currentBtn == 0 || currentBtn != lastPressedBtn) {
    lastPressedBtn = currentBtn; // Сбрасываем или переключаем
    isHolding = false;
    return index;
  }

  // ЛОГИКА 3: Кнопка удерживается
  if (currentBtn == lastPressedBtn) {
    // Проверяем, прошли ли стартовые 500 мс удержания
    if (!isHolding) {
      if (currentTime - btnPressedTime >= initialDelay) {
        isHolding = true;
        lastStepTime = currentTime; // Начинаем отсчет шагов автоповтора
      }
      return index; // Еще ждем 500 мс
    }

    // Если мы уже в режиме автоповтора, считаем текущую динамическую задержку
    // Время удержания после первых 500 мс
    unsigned long holdDuration = currentTime - btnPressedTime - initialDelay; 
    
    // Формула ускорения: плавно уменьшаем updateDelay с течением времени удержания
    long dynamicDelay = (long)updateDelay - (long)(holdDuration / accelerationSteps);
    if (dynamicDelay < (long)minDelay) {
      dynamicDelay = minDelay; // Ограничиваем максимальную скорость
    }

    // Проверяем, пора ли делать следующий шаг с учетом динамической задержки
    if (currentTime - lastStepTime >= (unsigned long)dynamicDelay) {
      lastStepTime = currentTime;

      if (currentBtn == 1 && index < max - 1) { index++; displayClear(); }
      else if (currentBtn == 2 && index > 0) { index--; displayClear(); }
    }
  }

  return index;
}
