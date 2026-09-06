#include "M5StickCPlus2.h"

#include "libs/gui/gui.h"
#include "libs/gui/activity_check.h"
#include "libs/gui/print_time.h"
#include "libs/gui/print_battery.h"
#include "apps/app.h"
#include "core/task_manager/task_manager.h"

#include "apps/home/home.h"
#include "apps/file_manager/file_manager.h"
#include "apps/web_server/server_frontend_app.h"
#include "apps/tv_b_gone/tv_b_gone.h"
#include "apps/calc/calc.h"
#include "apps/world_3d/world_3d.h"
#include "apps/settings/settings.h"


struct AppSlot {
  App* (*create)();
  const char* (*getName)();
  const char* (*startPrompt)();
  const bool (*autoStart)();
  App* active;
};

template<typename T>
constexpr AppSlot MakeAppSlot() {
  return AppSlot{
    []() -> App* { return new T(); },
    &T::GetName,
    &T::StartPrompt,
    &T::AutoStart,
    nullptr
  };
}

AppSlot appSlots[] = {
  MakeAppSlot<HomeApp>(),
  MakeAppSlot<FileManagerApp>(),
  MakeAppSlot<ServerFrontendApp>(),
  MakeAppSlot<TvbGoneApp>(),
  MakeAppSlot<CalcApp>(),
  MakeAppSlot<World3DApp>(),
  MakeAppSlot<SettingsApp>(),
};
const byte appCount = sizeof(appSlots) / sizeof(appSlots[0]);

byte selectedIndex = 0;
bool appRunning = false;


void setup() {
  auto cfg = M5.config();
  Serial.begin(115200);
  StickCP2.begin(cfg);

  pinMode(35, INPUT_PULLUP);
  pinMode(IR_TX_PIN, OUTPUT);

  StickCP2.Display.setRotation(3);
  
  for (auto &slot : appSlots) {
    if (slot.autoStart()) {
      slot.active = slot.create();
    }
  }

  updateActivity();
}


void handleLauncher() {
  AppSlot &slot = appSlots[selectedIndex];

  if (appRunning) {
    if (!slot.active->Loop()) {
      StickCP2.update();
      displayClear();
      updateActivity();
      slot.active->Exit();
      delete slot.active;
      slot.active = nullptr;
      appRunning = false;
    }
    return;
  }

  if (slot.autoStart()) {
    slot.active->Loop();
    return;
  }

  // Приложение ещё не запущено: показываем приглашение и ждём BtnA.
  displayBigText(slot.startPrompt());
  if (StickCP2.BtnA.wasPressed()) {
    displayClear();
    displayBigText("work...");
    slot.active = slot.create();
    slot.active->Setup();
    updateActivity();
    appRunning = true;
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
  displayAppName(appSlots[selectedIndex].getName());
  printBattery();
}


void loop() {
  StickCP2.update();

  TaskManager::Instance().LoopAll();

  handleLauncher();
  if (appRunning) { return; }

  selectedIndex = updateMenuSelection(selectedIndex, appCount);
  displayDockPanel();
  activityCheck();
  drawIdleTimerBar();
}
