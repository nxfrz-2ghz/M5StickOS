#include "Home.h"
#include "../../libs/gui/printTime.h"

HomeApp homeApp;

bool HomeApp::Loop() {
    printTime(true);
    return true;
}
