#include "Arduino.h"
#include "btnC.h"

#define BtnC 35

static bool btnC_lastState = false;
static bool btnC_wasPressedFlag = false;

// 1. ОДНА ФУНКЦИЯ ОБНОВЛЕНИЯ — вызывается строго один раз в начале loop()
void updateBtnC() {
    bool currentState = (digitalRead(BtnC) == LOW); // LOW = нажата
    
    // Флаг станет true ТОЛЬКО в момент перехода от "отпущена" к "нажата"
    btnC_wasPressedFlag = (currentState && !btnC_lastState);
    
    btnC_lastState = currentState; // Запоминаем состояние для следующего цикла
}

// 2. ФУНКЦИЯ-ГЕТТЕР — возвращает сохраненное состояние (вызывайте сколько угодно раз)
bool BtnC_wasPressed() {
    return btnC_wasPressedFlag;
}
