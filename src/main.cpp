#include "M5StickCPlus2.h"
#include "libs/gui/gui.h"
#include "libs/gui/activityCheck.h"
#include "libs/gui/printTime.h"
#include "libs/gui/printBattery.h"
#include "apps/App.h"
#include "apps/Home/Home.h"
#include "apps/FileManager/fileManager.h"
#include "apps/WebServer/server.h"
#include "apps/TV-B-Gone/tvbGone.h"
#include "apps/Calc/calc.h"
#include "apps/World3D/World3D.h"


void setup() {
  auto cfg = M5.config();
  Serial.begin(115200);
  StickCP2.begin(cfg);

  pinMode(35, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(IRLED, OUTPUT);

  StickCP2.Display.setRotation(3);

  update_activity();
}


App* apps[] = {
  &homeApp,
  &fileManagerApp,
  &webServerApp,
  &tvbGoneApp,
  &calcApp,
  &world3DApp,
};
const byte appCount = sizeof(apps) / sizeof(apps[0]);

byte selectedIndex = 0;
bool modalRunning = false;


void handleLauncher() {
  App* app = apps[selectedIndex];

  if (modalRunning) {
    if (!app->Loop()) {
      app->Exit();
      modalRunning = false;
    }
    return;
  }

  if (app->AutoStart()) {
    app->Loop();
    return;
  }

  // Приложение ещё не запущено: показываем приглашение и ждём BtnA.
  displayBigText(app->StartPrompt());
  if (StickCP2.BtnA.wasPressed()) {
    StickCP2.Display.fillRect(0, 0, StickCP2.Display.width(), StickCP2.Display.height(), BLACK);
    displayBigText("work...");
    app->Setup();
    update_activity();
    modalRunning = true;
  }
}


void displayAppName(const char* name) {
  StickCP2.Display.startWrite();
  StickCP2.Display.setFont(&fonts::FreeSansBold9pt7b);
  StickCP2.Display.setTextDatum(top_center);
  StickCP2.Display.setCursor(StickCP2.Display.width() / 4, 5);
  StickCP2.Display.printf("%s", name);
  StickCP2.Display.endWrite();
}


void displayDockPanel() {
  printTime(false);
  displayAppName(apps[selectedIndex]->GetName());
  printBattery();

  drawIdleTimerBar();
}


void loop() {
  StickCP2.update();

  handleLauncher();
  if (modalRunning) { return; }

  selectedIndex = updateMenuSelection(selectedIndex, appCount);
  displayDockPanel();
  activity_check();
}
