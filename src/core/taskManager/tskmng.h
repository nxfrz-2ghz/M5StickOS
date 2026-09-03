#pragma once

#include <vector>
#include "../../apps/Task.h"

// TaskManager — единая точка регистрации и диспетчеризации фоновых задач.
//
// Задачи регистрируются один раз при старте прошивки (Register), а
// запускаются/останавливаются по имени когда угодно и откуда угодно —
// например, из App::Setup() того GUI-приложения, которое такой задачей
// управляет ("frontend"). Именно это и позволяет одному frontend-приложению
// запустить задачу, закрыться (вернуться в лаунчер), а задаче — продолжать
// работать в фоне: другое (или то же самое) приложение может в любой
// момент вызвать Find() и получить указатель на уже работающий экземпляр,
// чтобы прочитать его состояние или подёргать его методы.
//
// main.cpp должен каждый тик вызывать TaskManager::Instance().LoopAll(),
// независимо от того, какое App сейчас активно на экране.
class TaskManager {
public:
    using Factory = Task* (*)();

    static TaskManager& Instance() {
        static TaskManager instance;
        return instance;
    }

    // Регистрирует задачу под именем `name`. Сама задача при этом ещё не
    // создаётся — только запоминается фабрика, которая создаст экземпляр
    // в момент Start().
    void Register(const char* name, Factory factory) {
        entries_.push_back({name, factory, nullptr});
    }

    // Запускает задачу, если она ещё не запущена (идемпотентно — повторный
    // вызов на уже работающей задаче ничего не делает и просто возвращает
    // существующий экземпляр). Возвращает nullptr, если задача с таким
    // именем не зарегистрирована.
    Task* Start(const char* name) {
        Entry* entry = find(name);
        if (!entry) return nullptr;
        if (!entry->instance) {
            entry->instance = entry->factory();
            entry->instance->Setup();
        }
        return entry->instance;
    }

    // Останавливает задачу (если она запущена) и уничтожает её экземпляр.
    void Stop(const char* name) {
        Entry* entry = find(name);
        if (!entry || !entry->instance) return;
        entry->instance->Stop();
        delete entry->instance;
        entry->instance = nullptr;
    }

    // Возвращает указатель на работающий экземпляр задачи или nullptr,
    // если она не запущена. Используется frontend-приложениями, чтобы
    // прочитать состояние задачи и вывести его на экран.
    Task* Find(const char* name) {
        Entry* entry = find(name);
        return entry ? entry->instance : nullptr;
    }

    bool IsRunning(const char* name) {
        return Find(name) != nullptr;
    }

    // Прогоняет Loop() всех запущенных задач. Если задача сама вернула
    // false — она останавливается и удаляется автоматически.
    void LoopAll() {
        for (auto &entry : entries_) {
            if (!entry.instance) continue;
            if (!entry.instance->Loop()) {
                entry.instance->Stop();
                delete entry.instance;
                entry.instance = nullptr;
            }
        }
    }

private:
    struct Entry {
        const char* name;
        Factory factory;
        Task* instance;
    };

    Entry* find(const char* name) {
        for (auto &entry : entries_) {
            if (strcmp(entry.name, name) == 0) return &entry;
        }
        return nullptr;
    }

    std::vector<Entry> entries_;
};
