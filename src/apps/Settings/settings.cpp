#include "settings.h"

#include "../../libs/gui/gui.h"

void SettingsApp::renderPage() {
    std::vector<String> displayLines;
    displayLines.reserve(settingsCount);

    for (size_t i = 0; i < settingsCount; i++) {
        byte offset = (selectedIndex + i) % settingsCount;
        String line = String(settingKeys[offset].data(), settingKeys[offset].size());
        
        switch (settingValues[i].type) {
            case ConfigValue::TYPE_BOOL:
                line += ": " + String(settingValues[offset].bVal ? "ON" : "OFF");
                break;
            case ConfigValue::TYPE_INT:
                line += ": " + String(settingValues[offset].iVal);
                break;
            case ConfigValue::TYPE_STRING:
                if (settingValues[offset].sVal.length() > 0) {
                    line += ": " + settingValues[offset].sVal;
                }
                // Строка значения пуста - значит это заголовок
                else {
                    line = "--- " + line + " ---";
                }
                break;
        }
        displayLines.push_back(line);
    }

    displayList("--- SETTINGS ---", displayLines, selectedIndex);
}

void SettingsApp::Setup() {
    displayClear();
    renderPage();
}

bool SettingsApp::Loop() {
    if (StickCP2.BtnPWR.wasPressed() && selectedIndex == 0) {
        return false;
    }
    byte lastIndex = selectedIndex;
    selectedIndex = updateMenuSelection(selectedIndex, settingsCount);

    if (selectedIndex != lastIndex) {
        renderPage();
    }

    return true;
}
