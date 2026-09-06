#include "home.h"
#include "../../libs/gui/print_time.h"

bool HomeApp::Loop() {
    printTime(true);
    return true;
}
