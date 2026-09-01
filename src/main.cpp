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


// Слот приложения в лаунчере.
//
// `prototype` — лёгкий, ничего не делающий экземпляр, который живёт всё
// время работы прошивки и нужен только для того, чтобы было у кого
// спросить GetName()/StartPrompt()/AutoStart() ещё до того, как
// приложение реально запущено (Setup() на нём никогда не вызывается).
//
// `create` — фабрика, создающая СВЕЖИЙ экземпляр приложения в момент
// запуска (BtnA на экране "< START >"). Именно на этом новом экземпляре
// и вызывается Setup()/Loop()/Exit() — поэтому классам приложений больше
// не нужно вручную сбрасывать своё состояние в Setup(): достаточно
// объявить поля с значениями по умолчанию прямо в классе (см. CalcApp,
// FileManagerApp, World3DApp).
//
// `active` — указатель на реально работающий экземпляр. Для AutoStart-
// приложений (Home, TV) он совпадает с prototype и создаётся один раз
// при старте прошивки: такие приложения "всегда включены" и не имеют
// сценария повторного запуска. Для остальных — создаётся при нажатии
// BtnA и уничтожается сразу после Exit().
struct AppSlot {
  AppSlot() : create(nullptr), prototype(nullptr), active(nullptr) {}
  AppSlot(App* (*factory)()) : create(factory), prototype(nullptr), active(nullptr) {}

  App* (*create)();
  App* prototype;
  App* active;
};

AppSlot appSlots[] = {
  AppSlot([]() -> App* { return new HomeApp(); }),
  AppSlot([]() -> App* { return new FileManagerApp(); }),
  AppSlot([]() -> App* { return new ServerFrontendApp(); }),
  AppSlot([]() -> App* { return new TvbGoneApp(); }),
  AppSlot([]() -> App* { return new CalcApp(); }),
  AppSlot([]() -> App* { return new World3DApp(); }),
  AppSlot([]() -> App* { return new SettingsApp(); }),
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

  // Каждому слоту заводим постоянный "прототип" — он живёт всё время
  // работы прошивки и никогда не проходит Setup()/Loop() сам по себе,
  // за исключением AutoStart-приложений, для которых prototype и есть
  // единственный работающий экземпляр (см. комментарий к AppSlot выше).
  for (auto &slot : appSlots) {
    slot.prototype = slot.create();
    if (slot.prototype->AutoStart()) {
      slot.active = slot.prototype;
    }
  }

  updateActivity();
}


void handleLauncher() {
  AppSlot &slot = appSlots[selectedIndex];

  if (modalRunning) {
    if (!slot.active->Loop()) {
      slot.active->Exit();
      if (slot.active != slot.prototype) {
        delete slot.active;
      }
      slot.active = nullptr;
      modalRunning = false;
    }
    return;
  }

  if (slot.prototype->AutoStart()) {
    slot.active->Loop();
    return;
  }

  // Приложение ещё не запущено: показываем приглашение и ждём BtnA.
  displayBigText(slot.prototype->StartPrompt());
  if (StickCP2.BtnA.wasPressed()) {
    StickCP2.Display.fillRect(0, 0, StickCP2.Display.width(), StickCP2.Display.height(), BLACK);
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
  displayAppName(appSlots[selectedIndex].prototype->GetName());
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
