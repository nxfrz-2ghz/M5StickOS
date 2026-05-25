#include "M5StickCPlus2.h"
#include <LittleFS.h>
#include <vector>
#include "gui.h"
#include "btnC.h"
#include "fileReader.h"
#include "fileManager.h"

static String currentPath = "/";
static std::vector<String> filesList;

// Состояния менеджера
enum ManagerState { LIST, FILE_OPTIONS, READER };
static ManagerState currentState = LIST;
static std::vector<String> options = {"Open", "Delete", "Close"};

static int selectedIndex = 0;
static int optionIndex = 0;


static void updateFilesArray(String path) {
    filesList.clear();
    File root = LittleFS.open(path);
    if (!root || !root.isDirectory()) return;

    File file = root.openNextFile();
    while (file) {
        filesList.push_back(String(file.name()));
        file = root.openNextFile();
    }
}

static void displayUI() {
    if (currentState == LIST) {
        // Используем твою новую функцию и для основного списка тоже!
        displayList("--- Files ---", filesList, selectedIndex);
    } else {
        // ИСПРАВЛЕНО: передаем вектор опций
        displayList("Actions:", options, optionIndex);
    }
}

static void handleAction() {
    if (optionIndex == 0) { // Открыть
        openTextFile(filesList[selectedIndex]);
        currentState = READER;
        return;
    } 
    else if (optionIndex == 1) { // Удалить
        String fullPath = currentPath + (currentPath.endsWith("/") ? "" : "/") + filesList[selectedIndex];
        if (LittleFS.remove(fullPath)) {
            displayText("Deleted!");
            updateFilesArray(currentPath);
            selectedIndex = 0;
        } else {
            displayText("Error delete");
        }
        delay(1000);
    }
    
    // После любого действия (или нажатия "Назад") возвращаемся в список
    currentState = LIST;
    optionIndex = 0;
    displayUI();
}

void initFileManager() {
    if (!LittleFS.begin(true)) {
        displayText("FS Mount Failed");
        return;
    }
    updateFilesArray(currentPath);
    displayUI();
}

void loopFileManager() {

    // Если запущен READER, то передаем весь контроль ему
    if (currentState == READER) {
        if (!loopFileReader()) {
            ESP.restart();
        }
        return;
    }

    
    // Navigation
    if (currentState == LIST && !filesList.empty()){
        int lastIndex = selectedIndex;

        if (BtnC_wasPressed() && lastIndex == 0){
            ESP.restart();
        }

        selectedIndex = updateMenuSelection(selectedIndex, filesList.size());

        if (lastIndex != selectedIndex){
            displayUI();
        }
    }
    else if (currentState == FILE_OPTIONS){
        int lastIndex = optionIndex;
        optionIndex = updateMenuSelection(optionIndex, options.size());

        if (lastIndex != optionIndex){
            displayUI();
        }
    }


    // Choose
    if (StickCP2.BtnA.wasPressed()) {
        if (currentState == LIST && !filesList.empty()) {
            currentState = FILE_OPTIONS;
            optionIndex = 0;
        } else if (currentState == FILE_OPTIONS) {
            handleAction();
            return; // displayUI уже вызван в handleAction
        }
        displayUI();
    }
}

