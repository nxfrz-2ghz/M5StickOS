#pragma once

#include "../App.h"
#include <cstdint>

class World3DApp : public App {
public:
    // Публичные, т.к. используются файловыми статиками в World3D.cpp
    // (например, mapObstacle) — самому классу они нужны только внутри.
    struct RgbColor { int r; int g; int b; };

    struct Obst {
        int x;
        int y;
        uint16_t color;
        RgbColor rgb;
    };

    static const char* GetName() { return "3D"; }
    static const char* StartPrompt() { return "< RUN >"; }

    void Setup() override;
    bool Loop() override;
    void Exit() override;

private:
    struct IsObs {
        bool status;
        Obst infos;
    };

    struct HBlock {
        int      distance = 0;
        int      beginWall = 0;
        int      beginGrass = 0;
        uint16_t color = 0;
    };

    // Раньше это была одна структура game — глобальная переменная,
    // общая на все запуски приложения. Из-за этого, например, позиция
    // игрока (player) и текущий угол обзора (directionDegree) не
    // сбрасывались к значениям по умолчанию при повторном входе в
    // приложение — сбрасывались вручную только nbObstacles и _3DInit
    // в Setup(). Теперь это поля объекта: новый World3DApp создаётся
    // при каждом запуске (см. main.cpp), поэтому все поля и так
    // получают значения по умолчанию, объявленные прямо здесь.
    struct GMap {
        uint16_t W = 0;
        uint16_t H = 0;
        int mapW = 55;
        int mapH = 55;
        int directionDegree = 30;
        int maxDegreeLine   = 60;
        int maxDistanceLine = 20;
        int blockSize = 5;
        HBlock historyBlocks[380];
        Obst player = { 30, 5, 0, {0, 0, 0} };
        int nbObstacles = 0;
        Obst* obstacles = nullptr;
        bool _3DInit = false;
    };

    GMap game;

    void addObstacleMap();
    IsObs isObstacle(int x, int y) const;
    Obst getCoord(int x, int y, int dist, float degree) const;
    void drawLine(uint16_t color);
    static uint16_t getColorFromDistance(RgbColor rgbColor, int distance, int maxDistance, int coef);
    void draw3DLine();
};
