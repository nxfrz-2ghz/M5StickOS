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


void displayList(const char* title, const std::vector<String> &list, int selIdx) {
  String output = title;
  output += "\n";

  if (list.empty()) {
    output += "[Empty]";
  }
  else {
    for (int i = 0; i < list.size(); i++) {
      if (selIdx >= 0) {
        output += (i == selIdx ? "> " : "");
      }
      output += list[i] + "\n";
    }
  }
  displayText(output.c_str());
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
  // Next
  if (StickCP2.BtnB.wasPressed()) {
    index = index + 1;
    if (index >= max) {
      index = max - 1;
    }
    else{ // Update Screen
      displayClear();
    }
  }
  // Previous
  if (StickCP2.BtnPWR.wasPressed()) {
    index = index - 1;
    if (index < 0) {
      index = 0;
    }
    else{ // Update Screen
      displayClear();
    }
  }

  return index;
}

int updateMenuSelectionFast(int index, const int max, const int updateDelay) {
  // Next
  if (StickCP2.BtnB.isPressed()) {
    index = index + 1;
    if (index >= max) {
      index = max - 1;
    }
    else{ // Update Screen
      displayClear();
    }
  }
  // Previous
  if (StickCP2.BtnPWR.isPressed()) {
    index = index - 1;
    if (index < 0) {
      index = 0;
    }
    else{ // Update Screen
      displayClear();
    }
  }
  delay(updateDelay);

  return index;
}

