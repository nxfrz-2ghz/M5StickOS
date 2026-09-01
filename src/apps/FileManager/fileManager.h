#pragma once

#include "../App.h"
#include <vector>
#include <Arduino.h>

int getUsagePercentage();

class FileManagerApp : public App {
public:
    const char* GetName() const override { return "Files"; }
    void Setup() override;
    bool Loop() override;
    const char* StartPrompt() const override;

private:
    enum ManagerState { LIST, FILE_OPTIONS, READER, IMAGE_VIEWER };

    bool isImageFile(const String &filename) const;
    bool isTextFile(const String &filename) const;
    void updateFilesArray(const String &path);
    void updateOptions();
    void displayUI();
    void handleAction();

    // Заполняется в StartPrompt() через snprintf — вызывается каждый кадр,
    // пока висит экран "нажмите A", поэтому обычный String означал бы
    // аллокацию на каждый кадр. Метод StartPrompt() отмечен const, поэтому
    // буфер должен быть mutable, чтобы можно было обновлять его без
    // изменения логического состояния объекта.
    mutable char startPromptBuf[24] = {0};

    // Состояние менеджера — теперь поля объекта, а не статики файла:
    // новый экземпляр создаётся при каждом запуске приложения (см.
    // main.cpp), поэтому они и так каждый раз начинают "с чистого листа".
    String currentPath = "/";
    std::vector<String> filesList;
    ManagerState currentState = LIST;
    std::vector<String> options;
    int selectedIndex = 0;
    int optionIndex = 0;
};
