#include "print_battery.h"

void print_battery() {
    uint16_t mv = StickCP2.Power.getBatteryVoltage();

    int scrW = StickCP2.Display.width();
    int scrH = StickCP2.Display.height();

    // Позиция и размер индикатора
    int iconW = 40;
    int iconH = 14;
    int iconX = scrW - iconW - 6; // справа с отступом
    int iconY = 6;

    // Нарисовать контур батареи
    StickCP2.Display.drawRect(iconX, iconY, iconW, iconH, TFT_WHITE);
    // Клемма
    StickCP2.Display.fillRect(iconX + iconW, iconY + iconH/4, 4, iconH/2, TFT_WHITE);

    // Процент заряда по напряжению (ограничиваем 0..100)
    int pct = map(constrain((int)mv, BAT_MIN_MV, BAT_MAX_MV), BAT_MIN_MV, BAT_MAX_MV, 0, 100);

    // Внутренний заполненный прямоугольник (с отступом 2px)
    int innerW = iconW - 4;
    int innerH = iconH - 4;
    int innerX = iconX + 2;
    int innerY = iconY + 2;

    int fillW = (innerW * pct) / 100;
    uint16_t fillColor = (pct > 50) ? TFT_GREEN : (pct > 25 ? TFT_YELLOW : TFT_RED);
    if (fillW > 0) {
        StickCP2.Display.fillRect(innerX, innerY, fillW, innerH, fillColor);
    }

    // Рисуем деления (тиковые линии) внутри корпуса
    for (int i = 1; i < BAT_TICKS; ++i) {
        int tx = innerX + (i * innerW) / BAT_TICKS;
        StickCP2.Display.drawLine(tx, innerY, tx, innerY + innerH, TFT_WHITE);
    }

    // Показать значение вольтажа слева от индикатора
    char buf[8];
    float v = mv / 1000.0f;
    snprintf(buf, sizeof(buf), "%.2fV", v);

    // Небольшой шрифт для текста
    StickCP2.Display.setTextFont(0);
    StickCP2.Display.setTextSize(1);
    StickCP2.Display.setTextDatum(middle_right);
    StickCP2.Display.setTextColor(TFT_WHITE, BLACK);
    StickCP2.Display.drawString(buf, innerX - 4, iconY + iconH/2);
}
