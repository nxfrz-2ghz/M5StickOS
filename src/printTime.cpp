#include "M5StickCPlus2.h"
#include "printTime.h"

void printTime(bool big) {
    auto dt = StickCP2.Rtc.getDateTime();
    StickCP2.Display.startWrite();
    
    if (big) {
        // 1. Выводим ВРЕМЯ (крупно)
        StickCP2.Display.setFont(&fonts::FreeSansBold18pt7b);
        StickCP2.Display.setTextDatum(middle_center);
        StickCP2.Display.setCursor(StickCP2.Display.width() / 4, StickCP2.Display.height() / 2);
        StickCP2.Display.printf("%02d:%02d:%02d", dt.time.hours, dt.time.minutes, dt.time.seconds);

        // 2. Выводим ДАТУ (мелко)
        StickCP2.Display.setFont(&fonts::FreeSans9pt7b); 
        // Смещаем дату ниже центра
        StickCP2.Display.setCursor(StickCP2.Display.width() / 3, (StickCP2.Display.height() / 3) * 2);
        StickCP2.Display.printf("%02d.%02d.%04d", dt.date.date, dt.date.month, dt.date.year);
    } 
    else {
        // Выводим только время мелким шрифтом
        StickCP2.Display.setFont(&fonts::FreeSans9pt7b);
        StickCP2.Display.setTextDatum(top_left);
        StickCP2.Display.setCursor(5, 5);
        StickCP2.Display.printf("%02d:%02d", dt.time.hours, dt.time.minutes);
    }

    StickCP2.Display.endWrite();
}
