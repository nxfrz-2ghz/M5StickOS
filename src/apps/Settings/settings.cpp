#include "settings.h"

#include "../../libs/gui/gui.h"

void SettingsApp::renderPage() {
    std::vector<String> displayLines;
    displayLines.reserve(settingsCount);

    for (size_t i = 0; i < settingsCount; ++i) {
        String line = String(settingKeys[i].data(), settingKeys[i].size());

        switch (settingValues[i].type) {
            case ConfigValue::TYPE_BOOL:
                line += ": " + String(settingValues[i].bVal ? "ON" : "OFF");
                break;
            case ConfigValue::TYPE_INT:
                line += ": " + String(settingValues[i].iVal);
                break;
            case ConfigValue::TYPE_STRING:
                if (settingValues[i].sVal.length() > 0) {
                    line += ": " + settingValues[i].sVal;
                } else {
                    line = "[[ " + line + " ]]";
                }
                break;
        }

        displayLines.push_back(line);
    }

    // Same navigation model as FileManager: selectedIndex is the absolute
    // index, while GUI::displayList() chooses the visible portion.
    displayList("--- SETTINGS ---", displayLines, selectedIndex, true);
}

void SettingsApp::Setup() {
    selectedIndex = 0;
    displayClear();
    renderPage();
}

bool SettingsApp::Loop() {
    if (StickCP2.BtnPWR.wasPressed() && selectedIndex == 0) {
        return false;
    }

    const byte oldIndex = selectedIndex;
    selectedIndex = updateMenuSelection(selectedIndex, settingsCount);

    if (oldIndex != selectedIndex) {
        renderPage();
    }

    return true;
}
