#pragma once

#include "../app.h"
#include <Arduino.h>

class TimerApp : public App {
public:
    static const char* GetName() { return "Timer"; }
    static const char* StartPrompt() { return "< TIMER >"; }

    void Setup() override;
    bool Loop() override;
    void Exit() override;

private:
    enum class State : uint8_t { STOPPED, RUNNING, PAUSED };

    static const int kMaxLevel = 10;
    static const int kMicSamples = 256;
    static const uint32_t kMicSampleRate = 16000;
    static const unsigned long ignoreMicUntilMs = 300;

    State state = State::STOPPED;

    unsigned long accumulatedMs = 0;
    unsigned long runStartMs = 0;

    int soundLevel = 0;
    bool micReady = false;
    unsigned long micReadyAt = 0;
    unsigned long disableMicUntilMs = 0;
    bool exitRequested = false;

    float lastMicLevel = 0.0f;

    int16_t micBuffer[kMicSamples];

    // Redraw cache
    String drawnTimeText;
    String drawnStatusLine;
    String drawnMicLine;
    bool drawnMicLineVisible = false;
    int drawnSoundLevel = -1;

    void startRun();
    void stopRun();
    void resetTimer();

    unsigned long elapsedMs() const;
    float soundThreshold() const;

    void handleButtons();
    void syncMicPower();
    void updateMic();
    void render();
    String formatElapsed(unsigned long ms) const;
};
