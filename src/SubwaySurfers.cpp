// ============================================================
//  SubwaySurfers.cpp
//  Псевдо-3D раннер для M5StickC Plus 2
//
//  Экран  : 135 × 240  (держим вертикально)
//  Гироскоп: MPU6886  (крен = управление полосой)
//  Библиотеки: M5StickCPlus2.h, M5GFX (встроена)
// ============================================================

#include <M5StickCPlus2.h>
#include "SubwaySurfers.h"

// ─── константы экрана ───────────────────────────────────────
static constexpr int SCR_W        = 135;
static constexpr int SCR_H        = 240;

// ─── перспектива / «камера сверху-сзади» ────────────────────
// Горизонт находится на Y = HORIZON_Y
// Дорожки сходятся к точке схода (VP_X, HORIZON_Y)
static constexpr int   VP_X       = SCR_W / 2;   // точка схода X
static constexpr int   HORIZON_Y  = 60;           // горизонт
static constexpr int   ROAD_BOT_Y = SCR_H - 10;  // низ дороги
static constexpr float ROAD_HALF_BOT = 58.0f;     // полуширина внизу
static constexpr float ROAD_HALF_TOP = 12.0f;     // полуширина вверху

// 3 полосы: -1 = лево, 0 = центр, 1 = право
static constexpr int   LANE_COUNT   = 3;
static constexpr float LANE_OFFSETS[LANE_COUNT] = { -1.0f, 0.0f, 1.0f };

// ─── параметры игры ─────────────────────────────────────────
static constexpr float INITIAL_SPEED   = 0.012f;  // скорость Z (0..1/кадр)
static constexpr float SPEED_INCREMENT = 0.000002f;
static constexpr float GYRO_THRESHOLD  = 12.0f;   // °/с для смены полосы
static constexpr int   LANE_SWITCH_MS  = 350;     // мс задержка смены
static constexpr int   MAX_OBSTACLES   = 8;
static constexpr int   MAX_COINS       = 10;
static constexpr int   INVINCIBLE_MS   = 1500;    // мс неуязвимости после удара
static constexpr int   MAX_LIVES       = 3;

// ─── цвета (RGB565) ─────────────────────────────────────────
static constexpr uint16_t C_SKY_TOP    = 0x0010;  // тёмно-синий
static constexpr uint16_t C_SKY_BOT    = 0x4A49;  // серо-голубой
static constexpr uint16_t C_ROAD       = 0x39E7;  // асфальт
static constexpr uint16_t C_ROAD_LINE  = 0xFFFF;  // разметка
static constexpr uint16_t C_PLAYER     = 0x07FF;  // голубой
static constexpr uint16_t C_OBSTACLE   = 0xF800;  // красный
static constexpr uint16_t C_COIN       = 0xFFE0;  // жёлтый
static constexpr uint16_t C_BUILDING_A = 0x2104;  // тёмно-серый
static constexpr uint16_t C_BUILDING_B = 0x4208;
static constexpr uint16_t C_WINDOW     = 0xFFE0;
static constexpr uint16_t C_HUD_BG     = 0x0000;
static constexpr uint16_t C_HUD_TEXT   = 0xFFFF;
static constexpr uint16_t C_HEART      = 0xF800;

// ─── структуры ──────────────────────────────────────────────
struct Obstacle {
    float z;          // 0=у горизонта, 1=у экрана
    int   lane;       // 0..2
    bool  active;
    uint8_t type;     // 0=барьер, 1=поезд(высокий), 2=монета-блок
};

struct Coin {
    float z;
    int   lane;
    bool  active;
    bool  collected;
};

struct Building {
    float z;          // 0..1
    int   side;       // -1 = лево, +1 = право
    int   width;      // условная ширина в мировых единицах
    int   height;     // условная высота
    uint16_t color;
    bool  active;
};

// ─── static переменные игры ─────────────────────────────────
static M5GFX    gfx;
static M5Canvas canvas(&M5.Lcd);   // двойная буферизация

// Состояние
static bool  gameOver      = false;
static bool  gameStarted   = false;
static int   score         = 0;
static int   hiScore       = 0;
static int   lives         = MAX_LIVES;
static float speed         = INITIAL_SPEED;

// Игрок
static int   playerLane    = 1;          // текущая полоса (0..2)
static int   targetLane    = 1;
static float playerT       = 0.95f;      // Z игрока (почти у низа)
static float laneBlend     = 0.0f;       // 0..1 анимация смены полосы
static float laneBlendDir  = 0.0f;

// Управление
static float  gyroBuf      = 0.0f;
static uint32_t lastLaneSwitchMs = 0;

// Объекты
static Obstacle obstacles[MAX_OBSTACLES];
static Coin     coins[MAX_COINS];
static Building buildings[6];            // 3 слева + 3 справа

// Таймеры
static uint32_t lastObstacleMs  = 0;
static uint32_t lastCoinMs      = 0;
static uint32_t lastBuildingMs  = 0;
static uint32_t invincibleUntil = 0;
static uint32_t frameMs         = 0;
static uint32_t lastFrameMs     = 0;

// ─── утилиты перспективы ────────────────────────────────────

// z=0 → горизонт, z=1 → низ экрана
// возвращает Y в пикселях
static int perspY(float z) {
    return (int)(HORIZON_Y + (ROAD_BOT_Y - HORIZON_Y) * z);
}

// возвращает X для заданной полосы (lane -1..1 float) при заданном z
static int perspX(float laneF, float z) {
    float halfW = ROAD_HALF_TOP + (ROAD_HALF_BOT - ROAD_HALF_TOP) * z;
    float laneW  = halfW * 2.0f / LANE_COUNT;
    float centerX = VP_X + laneF * laneW;
    return (int)centerX;
}

// масштаб объекта при глубине z
static float perspScale(float z) {
    return 0.08f + 0.92f * z;   // линейная аппроксимация
}

// ─── инициализация объектов ─────────────────────────────────
static void resetObstacles() {
    for (int i = 0; i < MAX_OBSTACLES; i++) obstacles[i].active = false;
}
static void resetCoins() {
    for (int i = 0; i < MAX_COINS; i++) coins[i].active = false;
}
static void resetBuildings() {
    for (int i = 0; i < 6; i++) buildings[i].active = false;
}

static void spawnObstacle() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) {
            obstacles[i].active = true;
            obstacles[i].z      = 0.0f;
            obstacles[i].lane   = random(0, LANE_COUNT);
            obstacles[i].type   = (uint8_t)random(0, 3);
            return;
        }
    }
}

static void spawnCoin() {
    // Спавним 3 монеты подряд по одной полосе
    int lane = random(0, LANE_COUNT);
    float baseZ = 0.0f;
    int placed = 0;
    for (int i = 0; i < MAX_COINS && placed < 3; i++) {
        if (!coins[i].active) {
            coins[i].active    = true;
            coins[i].collected = false;
            coins[i].z         = baseZ + placed * 0.04f;
            coins[i].lane      = lane;
            placed++;
        }
    }
}

static void spawnBuilding() {
    uint16_t col = (random(2) == 0) ? C_BUILDING_A : C_BUILDING_B;
    for (int side : {-1, 1}) {
        for (int i = 0; i < 6; i++) {
            if (!buildings[i].active) {
                buildings[i].active = true;
                buildings[i].z      = 0.0f;
                buildings[i].side   = side;
                buildings[i].width  = random(20, 40);
                buildings[i].height = random(30, 80);
                buildings[i].color  = col;
                break;
            }
        }
    }
}

// ─── сброс игры ─────────────────────────────────────────────
// Вынесено из loop(): static внутри функции не сбрасывается при повторном вызове
static uint32_t lastScoreMs = 0;

static void resetGame() {
    score           = 0;
    lives           = MAX_LIVES;
    speed           = INITIAL_SPEED;
    playerLane      = 1;
    targetLane      = 1;
    laneBlend       = 0.0f;
    laneBlendDir    = 0.0f;
    gameOver        = false;
    gameStarted     = false;   // ← возврат на Title screen, drawTitle() вызовется в loop
    gyroBuf         = 0.0f;
    invincibleUntil = 0;
    // Все таймеры через одно millis() — иначе первый dt после рестарта = время игры
    uint32_t now    = millis();
    lastObstacleMs  = now;
    lastCoinMs      = now;
    lastBuildingMs  = now;
    lastScoreMs     = now;
    lastFrameMs     = now;     // ← главный фикс: без этого dt огромный, объекты мгновенно улетают
    resetObstacles();
    resetCoins();
    resetBuildings();
}

// ─── рисование неба с градиентом ────────────────────────────
static void drawSky() {
    // Простой градиент: несколько полос
    for (int y = 0; y < HORIZON_Y; y++) {
        float t  = (float)y / HORIZON_Y;
        uint8_t r = (uint8_t)(0   + t * 30);
        uint8_t g = (uint8_t)(0   + t * 50);
        uint8_t b = (uint8_t)(16  + t * 80);
        canvas.drawFastHLine(0, y, SCR_W,
            canvas.color565(r, g, b));
    }
    // Горизонтальная «линия горизонта»
    canvas.drawFastHLine(0, HORIZON_Y, SCR_W, 0x4228);
}

// ─── рисование дороги ───────────────────────────────────────
static void drawRoad() {
    // Трапециевидная дорога
    for (int y = HORIZON_Y; y < SCR_H; y++) {
        float t  = (float)(y - HORIZON_Y) / (ROAD_BOT_Y - HORIZON_Y);
        float halfW = ROAD_HALF_TOP + (ROAD_HALF_BOT - ROAD_HALF_TOP) * t;
        int x0 = (int)(VP_X - halfW);
        int x1 = (int)(VP_X + halfW);
        // затемнение по глубине
        uint8_t bright = (uint8_t)(20 + t * 45);
        canvas.drawFastHLine(x0, y, x1 - x0,
            canvas.color565(bright, bright, bright));
    }

    // Разметка: 2 продольные линии (границы полос)
    for (int div = 1; div < LANE_COUNT; div++) {
        float laneF = -1.0f + div * (2.0f / LANE_COUNT);
        for (int y = HORIZON_Y; y <= ROAD_BOT_Y; y += 6) {
            float t = (float)(y - HORIZON_Y) / (ROAD_BOT_Y - HORIZON_Y);
            int x   = perspX(laneF, t);
            int len = max(1, (int)(4 * t));
            canvas.drawFastVLine(x, y, len, C_ROAD_LINE);
        }
    }
}

// ─── рисование зданий ───────────────────────────────────────
static void drawBuildings() {
    for (int i = 0; i < 6; i++) {
        if (!buildings[i].active) continue;
        Building &b = buildings[i];
        float t = b.z;
        float sc = perspScale(t);

        // X позиция — за краем дороги
        float halfRoad = ROAD_HALF_TOP + (ROAD_HALF_BOT - ROAD_HALF_TOP) * t;
        int roadEdge   = (int)(VP_X + b.side * halfRoad);
        int bw = (int)(b.width  * sc);
        int bh = (int)(b.height * sc);
        int by = perspY(t) - bh;

        int bx = (b.side > 0) ? roadEdge : roadEdge - bw;

        canvas.fillRect(bx, by, bw, bh, b.color);

        // Окна
        int wSize = max(2, (int)(4 * sc));
        int wGap  = max(3, (int)(6 * sc));
        for (int wy = by + wGap; wy < by + bh - wSize; wy += wGap + wSize) {
            for (int wx = bx + wGap; wx < bx + bw - wSize; wx += wGap + wSize) {
                if (random(3) != 0)  // не все окна светятся
                    canvas.fillRect(wx, wy, wSize, wSize, C_WINDOW);
            }
        }
    }
}

// ─── рисование препятствий ──────────────────────────────────
static void drawObstacles() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) continue;
        Obstacle &ob = obstacles[i];
        float sc = perspScale(ob.z);

        // Определяем лейн с учётом анимации перехода (для наглядности)
        float laneF = LANE_OFFSETS[ob.lane];
        int   cx    = perspX(laneF, ob.z);
        int   cy    = perspY(ob.z);

        int w = (int)(28 * sc);
        int h = (ob.type == 1) ? (int)(50 * sc) : (int)(22 * sc);
        int x = cx - w / 2;
        int y = cy - h;

        uint16_t col = (ob.type == 1) ? 0xF81F : C_OBSTACLE;
        canvas.fillRect(x, y, w, h, col);

        // «Мигание» при неуязвимости (если это препятствие рядом с игроком)
        if (millis() < invincibleUntil) {
            if ((millis() / 100) % 2 == 0)
                canvas.drawRect(x, y, w, h, 0xFFFF);
        }
    }
}

// ─── рисование монет ────────────────────────────────────────
static void drawCoins() {
    for (int i = 0; i < MAX_COINS; i++) {
        if (!coins[i].active || coins[i].collected) continue;
        float sc = perspScale(coins[i].z);
        float laneF = LANE_OFFSETS[coins[i].lane];
        int cx = perspX(laneF, coins[i].z);
        int cy = perspY(coins[i].z) - (int)(8 * sc);
        int r  = max(2, (int)(5 * sc));
        canvas.fillCircle(cx, cy, r, C_COIN);
        canvas.drawCircle(cx, cy, r, 0xFFA0);
    }
}

// ─── рисование игрока ───────────────────────────────────────
static void drawPlayer() {
    // Вычисляем текущий X с анимацией смены полосы
    float curLane = LANE_OFFSETS[playerLane] + laneBlend * laneBlendDir * (2.0f / LANE_COUNT);
    int cx  = perspX(curLane, playerT);
    int cy  = perspY(playerT);
    float sc = perspScale(playerT);

    int bodyW = (int)(18 * sc);
    int bodyH = (int)(28 * sc);
    int headR = (int)(7  * sc);

    // Мигание при неуязвимости
    bool visible = true;
    if (millis() < invincibleUntil && (millis() / 120) % 2 == 0)
        visible = false;

    if (visible) {
        // Тело
        canvas.fillRect(cx - bodyW/2, cy - bodyH, bodyW, bodyH, C_PLAYER);
        // Голова
        canvas.fillCircle(cx, cy - bodyH - headR, headR, 0xFFFF);
        // Ноги (анимация бега)
        int legPhase = (millis() / 150) % 2;
        int legW = (int)(5 * sc);
        int legH = (int)(10 * sc);
        canvas.fillRect(cx - bodyW/2 + 2, cy, legW, legH - (legPhase ? legH/2 : 0), 0x001F);
        canvas.fillRect(cx + bodyW/2 - legW - 2, cy, legW, legH - (legPhase ? 0 : legH/2), 0x001F);
    }
}

// ─── HUD (жизни + счёт) ─────────────────────────────────────
static void drawHUD() {
    // Верхняя полоска
    canvas.fillRect(0, 0, SCR_W, 16, C_HUD_BG);
    canvas.setTextColor(C_HUD_TEXT, C_HUD_BG);
    canvas.setTextSize(1);

    // Счёт
    canvas.setCursor(2, 4);
    canvas.printf("SC:%d", score);

    // Рекорд
    canvas.setCursor(50, 4);
    canvas.printf("HI:%d", hiScore);

    // Жизни (сердечки)
    for (int i = 0; i < lives; i++) {
        canvas.fillCircle(116 + i * 0, 8, 4, C_HEART);  // все в ряд не влезут — рисуем сжато
    }
    // Компактно: текст жизней
    canvas.setCursor(108, 4);
    canvas.setTextColor(C_HEART, C_HUD_BG);
    canvas.printf("x%d", lives);
}

// ─── экран Game Over ────────────────────────────────────────
static void drawGameOver() {
    canvas.fillScreen(0x0000);
    canvas.setTextColor(0xF800, 0x0000);
    canvas.setTextSize(2);
    canvas.setCursor(10, 70);
    canvas.print("GAME OVER");

    canvas.setTextColor(0xFFFF, 0x0000);
    canvas.setTextSize(1);
    canvas.setCursor(20, 110);
    canvas.printf("Score: %d", score);
    canvas.setCursor(20, 125);
    canvas.printf("Best:  %d", hiScore);

    canvas.setTextColor(0xFFE0, 0x0000);
    canvas.setCursor(18, 155);
    canvas.print("Press BtnA");
    canvas.setCursor(18, 168);
    canvas.print("to restart");

    canvas.pushSprite(0, 0);
}

// ─── экран Title ────────────────────────────────────────────
static void drawTitle() {
    canvas.fillScreen(0x0000);

    // Небо-градиент
    drawSky();
    drawRoad();

    canvas.setTextColor(0xFFE0, TFT_TRANSPARENT);
    canvas.setTextDatum(MC_DATUM);
    canvas.setTextSize(2);
    canvas.drawString("SURF", SCR_W/2, 80);
    canvas.drawString("CITY", SCR_W/2, 100);

    canvas.setTextColor(0xFFFF, TFT_TRANSPARENT);
    canvas.setTextSize(1);
    canvas.drawString("Tilt to move", SCR_W/2, 140);
    canvas.drawString("Press BtnA", SCR_W/2, 165);
    canvas.drawString("to start!", SCR_W/2, 178);

    canvas.setTextDatum(TL_DATUM);  // сбрасываем выравнивание
    canvas.pushSprite(0, 0);
}

// ─── обновление физики ──────────────────────────────────────
static void updateObjects(float dt) {
    float ds = speed;  // сколько «пространства» прошли за кадр

    // Препятствия
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) continue;
        obstacles[i].z += ds;
        if (obstacles[i].z >= 1.05f) obstacles[i].active = false;
    }

    // Монеты
    for (int i = 0; i < MAX_COINS; i++) {
        if (!coins[i].active) continue;
        coins[i].z += ds;
        if (coins[i].z >= 1.05f) coins[i].active = false;
    }

    // Здания
    for (int i = 0; i < 6; i++) {
        if (!buildings[i].active) continue;
        buildings[i].z += ds * 0.7f;  // чуть медленнее
        if (buildings[i].z >= 1.05f) buildings[i].active = false;
    }
}

// ─── столкновения ───────────────────────────────────────────
static void checkCollisions() {
    if (millis() < invincibleUntil) return;

    // Текущая «реальная» полоса игрока
    int pl = playerLane;

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) continue;
        if (obstacles[i].lane != pl) continue;
        // Проверяем близость по Z
        float dz = obstacles[i].z - playerT;
        if (dz > -0.06f && dz < 0.06f) {
            // Удар!
            lives--;
            invincibleUntil = millis() + INVINCIBLE_MS;
            if (lives <= 0) {
                gameOver = true;
                if (score > hiScore) hiScore = score;
            }
            obstacles[i].active = false;
            return;
        }
    }

    // Сбор монет
    for (int i = 0; i < MAX_COINS; i++) {
        if (!coins[i].active || coins[i].collected) continue;
        if (coins[i].lane != pl) continue;
        float dz = coins[i].z - playerT;
        if (dz > -0.05f && dz < 0.05f) {
            coins[i].collected = true;
            score++;
        }
    }
}

// ─── управление гироскопом ──────────────────────────────────
static float gyroBias = 0.0f;
static void handleGyro() {
    if (!StickCP2.Imu.update()) return;
 
    auto imu_data = StickCP2.Imu.getImuData();
    float raw = imu_data.gyro.x;
 
    if (fabsf(raw) < GYRO_THRESHOLD * 0.5f) {
        gyroBias = gyroBias * 0.999f + raw * 0.001f;
    }
 
    float corrected = raw - gyroBias;
 
    gyroBuf = gyroBuf * 0.5f + corrected * 0.5f;
 
    uint32_t now = millis();
    if (now - lastLaneSwitchMs < LANE_SWITCH_MS) return;
 
    if (gyroBuf > GYRO_THRESHOLD && playerLane < LANE_COUNT - 1) {
        playerLane++;
        lastLaneSwitchMs = now;
    } else if (gyroBuf < -GYRO_THRESHOLD && playerLane > 0) {
        playerLane--;
        lastLaneSwitchMs = now;
    }
}


// ─── спавн объектов по таймеру ──────────────────────────────
static void handleSpawns() {
    uint32_t now = millis();

    // Интервал спавна уменьшается со скоростью
    uint32_t obstInterval = (uint32_t)(2200 - speed * 40000);
    obstInterval = max(obstInterval, (uint32_t)600);

    if (now - lastObstacleMs > obstInterval) {
        spawnObstacle();
        lastObstacleMs = now;
    }

    uint32_t coinInterval = (uint32_t)(1800 - speed * 20000);
    coinInterval = max(coinInterval, (uint32_t)800);

    if (now - lastCoinMs > coinInterval) {
        spawnCoin();
        lastCoinMs = now;
    }

    if (now - lastBuildingMs > 1200) {
        spawnBuilding();
        lastBuildingMs = now;
    }
}

// ─── публичные функции ──────────────────────────────────────

void setupSurfGame() {
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    // IMU инициализируется внутри begin() — отдельный Imu.begin() не нужен.

    StickCP2.Display.setRotation(0);   // вертикальный портрет
    StickCP2.Display.fillScreen(TFT_BLACK);

    canvas.createSprite(SCR_W, SCR_H);
    canvas.setFont(&fonts::FreeSansBold9pt7b);

    randomSeed(analogRead(0));
    resetGame();

    gameStarted = false;
    drawTitle();
}

void loopSurfGame() {
    StickCP2.update();
    uint32_t now = millis();
    float dt = (float)(now - lastFrameMs) / 1000.0f;
    if (dt > 0.05f) dt = 0.05f;   // cap at 20fps minimum
    lastFrameMs = now;

    // ── Tittle Screen ──────────────────────────────────────
    if (!gameStarted) {
        if (StickCP2.BtnA.wasPressed()) {
            resetGame();          // сбрасывает всё, включая gameStarted = false
            gameStarted = true;   // ← ПОСЛЕ resetGame, иначе будет перезатёрт
        }
        if (StickCP2.BtnB.wasPressed()) {
            ESP.restart();
        }
        return;
    }

    // ── Game Over Screen ───────────────────────────────────
    if (gameOver) {
        if (StickCP2.BtnA.wasPressed()) {
            resetGame();          // gameStarted = false внутри
            drawTitle();          // показываем Title screen сразу
        }
        if (StickCP2.BtnB.wasPressed()) {
            ESP.restart();
        }
        drawGameOver();
        return;
    }

    // ── Игровая логика ────────────────────────────────────
    handleGyro();
    handleSpawns();
    updateObjects(dt);
    checkCollisions();

    // Увеличение скорости
    speed += SPEED_INCREMENT;

    // Счёт за дистанцию — каждые 500 мс +1
    if (now - lastScoreMs >= 500) {
        score++;
        lastScoreMs = now;
    }

    // ── Рендеринг ─────────────────────────────────────────
    canvas.fillScreen(0x0000);

    drawSky();
    drawBuildings();
    drawRoad();
    drawCoins();
    drawObstacles();
    drawPlayer();
    drawHUD();

    canvas.pushSprite(0, 0);

    // Небольшая задержка чтобы не перегревать CPU
    delay(16);  // ~60 FPS cap
}
