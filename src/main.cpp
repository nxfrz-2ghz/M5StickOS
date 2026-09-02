#include "M5StickCPlus2.h"
#include "libs/gui/gui.h"
#include "libs/gui/activityCheck.h"
#include "libs/gui/printTime.h"
#include "libs/gui/printBattery.h"
#include "apps/App.h"
#include "apps/TaskManager.h"
#include "apps/Home/Home.h"
#include "apps/FileManager/fileManager.h"
#include "apps/WebServer/serverFrontendApp.h"
#include "apps/TV-B-Gone/tvbGone.h"
#include "apps/Calc/calc.h"
#include "apps/World3D/World3D.h"
#include "apps/Settings/settings.h"


struct AppSlot {
  App* (*create)();
  const char* (*getName)();
  const char* (*startPrompt)();
  bool (*autoStart)();
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
bool modalRunning = false;


void setup() {
  auto cfg = M5.config();
  Serial.begin(115200);
  StickCP2.begin(cfg);

  pinMode(35, INPUT_PULLUP);
  pinMode(IR_TX_PIN, OUTPUT);

  StickCP2.Display.setRotation(3);

  // AutoStart-приложения (Home, TV) запускаем один раз прямо здесь —
  // созданный экземпляр и есть единственный работающий на всё время
  // работы прошивки (см. комментарий к AppSlot выше).
  for (auto &slot : appSlots) {
    if (slot.autoStart()) {
      slot.active = slot.create();
    }
  }

  updateActivity();
}


void handleLauncher() {
  AppSlot &slot = appSlots[selectedIndex];

  if (modalRunning) {
    if (!slot.active->Loop()) {
      StickCP2.update();
      displayClear();
      updateActivity();
      slot.active->Exit();
      delete slot.active;
      slot.active = nullptr;
      modalRunning = false;
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
  displayAppName(appSlots[selectedIndex].getName());
  printBattery();

  drawIdleTimerBar();
}


void loop() {
  StickCP2.update();

  // Фоновые задачи (см. apps/Task.h, apps/TaskManager.h) крутятся
  // каждый тик независимо от того, какое приложение сейчас на экране.
  TaskManager::Instance().LoopAll();

  handleLauncher();
  if (modalRunning) { return; }

  selectedIndex = updateMenuSelection(selectedIndex, appCount);
  displayDockPanel();
  activityCheck();
}
