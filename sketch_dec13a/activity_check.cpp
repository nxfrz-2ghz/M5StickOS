#include "activity_check.h"

#define timeout 5000 // 5 секунд
unsigned long lastActivityTime = 0;


int lastBarWidth = -1; // Глобальная или static переменная

void draw_idle_timer_bar() {
  unsigned long elapsed = millis() - lastActivityTime;
  float progress = 1.0 - (float)elapsed / timeout;
  if (progress < 0.0) progress = 0.0;

  int totalWidth = StickCP2.Display.width();
  int barWidth = (int)(totalWidth * progress);
  int barHeight = 3;
  int barY = StickCP2.Display.height() - barHeight;

  // Рисуем только если ширина изменилась
  if (barWidth != lastBarWidth) {
    // Закрашиваем только "исчезнувшую" часть полоски черным
    if (barWidth < lastBarWidth) {
      StickCP2.Display.fillRect(barWidth, barY, totalWidth - barWidth, barHeight, BLACK);
    }

    // Рисуем саму полоску
    uint16_t barColor = (progress > 0.5) ? TFT_GREEN: (progress > 0.25 ? TFT_YELLOW: TFT_RED);
    StickCP2.Display.fillRect(0, barY, barWidth, barHeight, barColor);

    lastBarWidth = barWidth;
  }
}


void update_activity() {
  lastActivityTime = millis();
}


void activity_check() {
  // Если нажата любая кнопка, обновляем время активности
  if (StickCP2.BtnA.wasPressed() || StickCP2.BtnB.wasPressed() || BtnC_wasPressed()) {
    update_activity();
  }

  // Проверка таймера бездействия
  if (millis() - lastActivityTime > timeout) {
    StickCP2.Display.fillScreen(BLACK);
    StickCP2.Display.drawString("OFF", StickCP2.Display.width()/2, StickCP2.Display.height()/2);
    delay(500);
    StickCP2.Power.powerOff(); // Выключение питания
  }
}