#include "home.h"
#include "../../libs/gui/print_time.h"

unsigned long HomeApp::lastTempUpd = 0;
char HomeApp::coreTemp[16] = "check";

bool HomeApp::Loop() {
    printTime(true);

    // Temp Update
    if (millis() - lastTempUpd > tempUpdInterval){
        snprintf(coreTemp, sizeof(coreTemp), "t=%.1f", temperatureRead());
        lastTempUpd = millis();
    }

    return true;
}
