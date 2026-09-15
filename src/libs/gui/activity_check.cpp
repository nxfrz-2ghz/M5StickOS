#include "M5StickCPlus2.h"
#include "activity_check.h"
#include "gui.h"

#define timeout 5000
static unsigned long lastActivityTime = 0;

void drawIdleTimerBar() {
  unsigned long elapsed = millis() - lastActivityTime;
  float progress = 1.0 - (float)elapsed / timeout;
  if (progress < 0.0) progress = 0.0;

  displayProgressBar(progress);
}

bool anyButtonPressed() {
	return (StickCP2.BtnA.isPressed() || StickCP2.BtnB.isPressed() || StickCP2.BtnPWR.isPressed());
}

void updateActivity() {
  lastActivityTime = millis();
}

void activityCheck() {
  if (anyButtonPressed()) {
    updateActivity();
  }

  // Проверка таймера бездействия
  if (millis() - lastActivityTime > timeout) {
    StickCP2.Display.fillScreen(BLACK);
    StickCP2.Display.drawString("OFF", StickCP2.Display.width()/2, StickCP2.Display.height()/2);
    delay(500);
    StickCP2.Power.powerOff(); // Выключение питания
  }
}