#include "M5StickCPlus2.h"
#include "btnC.h"
#include "gui.h"
#include "activityCheck.h"
#include "printTime.h"
#include "printBattery.h"
#include "tvbGone.h"
#include "server.h"
#include "fileManager.h"
#include "preferences.h"
#include "SubwaySurfers.h"


void setup() {
  auto cfg = M5.config();
  Serial.begin(115200);
  StickCP2.begin(cfg);

  pinMode(LED, OUTPUT);

  StickCP2.Display.setRotation(3);

  update_activity();
  pref_init();
}


// Application launcher state
bool isAppRunning = false;
String appList[] = {
  "home",
  "files",
  "webserver",
  "tvbgone",
  "SubwaySurf",
};
int selectedIndex = 0;
const int appCount = sizeof(appList) / sizeof(appList[0]);

// Launcher: handle the selected app's main action
void handleLauncher() {

  // Launcher
  if (appList[selectedIndex] == "home") {
    print_time(true);
  }
  else if (appList[selectedIndex] == "tvbgone") {
    displayBigText("< READY >");
    if (StickCP2.BtnA.wasPressed()) {
      StickCP2.Display.fillRect(0, 0, StickCP2.Display.width(), StickCP2.Display.height(), BLACK);
      displayBigText("work...");
      sendIRCodes();
      update_activity();
    }
  }
  else if (appList[selectedIndex] == "webserver") {
    if (isAppRunning){loopWebServer();}
    else{
      displayBigText("< START >");
      if (StickCP2.BtnA.wasPressed()) {
        StickCP2.Display.fillRect(0, 0, StickCP2.Display.width(), StickCP2.Display.height(), BLACK);
        displayBigText("work...");
        initWebServer();
        update_activity();
        isAppRunning = true;
      }
    }
  }
  else if (appList[selectedIndex] == "files") {
    if (isAppRunning){loopFileManager();}
    else{
      displayBigText("< BROWSE >");
      if (StickCP2.BtnA.wasPressed()) {
        StickCP2.Display.fillRect(0, 0, StickCP2.Display.width(), StickCP2.Display.height(), BLACK);
        displayBigText("work...");
        initFileManager();
        update_activity();
        isAppRunning = true;
      }
    }
  }
  else if (appList[selectedIndex] == "SubwaySurf") {
    if (isAppRunning){loopSurfGame();}
    else{
      displayBigText("< RUN >");
      if (StickCP2.BtnA.wasPressed()) {
        StickCP2.Display.fillRect(0, 0, StickCP2.Display.width(), StickCP2.Display.height(), BLACK);
        displayBigText("work...");
        setupSurfGame();
        update_activity();
        isAppRunning = true;
      }
    }
  }
}


void displayAppName(const String &name) {
  StickCP2.Display.startWrite();
  StickCP2.Display.setFont(&fonts::FreeSansBold9pt7b);
  StickCP2.Display.setTextDatum(top_center);
  StickCP2.Display.setCursor(StickCP2.Display.width() / 4, 5);
  StickCP2.Display.printf("%s", name.c_str());
  StickCP2.Display.endWrite();
}


void displayDockPanel() {
  print_time(false);
  displayAppName(appList[selectedIndex]);
  print_battery();

  draw_idle_timer_bar();
}


void loop() {
  StickCP2.update(); // Опрос кнопок
  updateBtnC();

  handleLauncher();
  if (isAppRunning){return;}

  selectedIndex = updateMenuSelection(selectedIndex, appCount);
  displayDockPanel();
  activity_check();

}
