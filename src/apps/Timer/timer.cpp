#include "timer.h"
#include "../../libs/gui/gui.h"

namespace {
constexpr int kStatusLineY = 4;
constexpr int kStatusLineH = 18;
constexpr int kMicLineY = 24;
constexpr int kMicLineH = 18;
constexpr int kTimeBandH = 40;
}

void TimerApp::startRun() {
    runStartMs = millis();
    state = State::RUNNING;
}

void TimerApp::stopRun() {
    accumulatedMs += millis() - runStartMs;
    state = State::PAUSED;
}

void TimerApp::resetTimer() {
    accumulatedMs = 0;
    state = State::STOPPED;
}

unsigned long TimerApp::elapsedMs() const {
    if (state == State::RUNNING) {
        return accumulatedMs + (millis() - runStartMs);
    }
    return accumulatedMs;
}

float TimerApp::soundThreshold() const {
    return 0.03f + ((float)soundLevel / (float)kMaxLevel) * 0.95f;
}

String TimerApp::formatElapsed(unsigned long ms) const {
    unsigned long totalTenths = ms / 100;
    unsigned long tenths = totalTenths % 10;
    unsigned long totalSeconds = totalTenths / 10;
    unsigned long seconds = totalSeconds % 60;
    unsigned long minutes = totalSeconds / 60;

    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu.%lu", minutes, seconds, tenths);
    return String(buf);
}

void TimerApp::Setup() {
    state = State::STOPPED;
    accumulatedMs = 0;
    runStartMs = 0;
    soundLevel = 0;
    exitRequested = false;
    lastMicLevel = 0.0f;

    micReady = false;
    micReadyAt = 0;

    GUI::clear();
}

void TimerApp::Exit() {
    if (micReady) {
        StickCP2.Mic.end();
        micReady = false;
    }
}

void TimerApp::syncMicPower() {
    if (soundLevel > 0 && !micReady) {
        micReady = StickCP2.Mic.begin();
        micReadyAt = millis();
    } else if (soundLevel == 0 && micReady) {
        StickCP2.Mic.end();
        micReady = false;
        lastMicLevel = 0.0f;
    }
}

void TimerApp::handleButtons() {
    if (StickCP2.BtnA.wasClicked()) {
        switch (state) {
            case State::STOPPED:
                startRun();
                break;
            case State::RUNNING:
                stopRun();
                break;
            case State::PAUSED:
                resetTimer();
                break;
        }
        disableMicUntilMs = millis() + ignoreMicUntilMs;
    }

    if (soundLevel == 0 && StickCP2.BtnPWR.wasClicked()) {
        exitRequested = true;
        return;
    }


    int newLevel = GUI::GUI::updateMenuSelection(soundLevel, kMaxLevel + 1, 150);
    if (newLevel != soundLevel) {
        soundLevel = newLevel;
        syncMicPower();
    }
}

void TimerApp::updateMic() {
    if (soundLevel <= 0 || !micReady) { // Mic disabled
        return;
    }

    if (millis() < disableMicUntilMs) { // Ignore first few samples after enabling mic
        return;
    }

    if (!StickCP2.Mic.record(micBuffer, kMicSamples, kMicSampleRate)) {
        return;
    }

    int16_t peak = 0;
    for (int i = 0; i < kMicSamples; ++i) {
        int16_t sample = micBuffer[i];
        int16_t amplitude = (sample < 0) ? -sample : sample;
        if (amplitude > peak) {
            peak = amplitude;
        }
    }

    lastMicLevel = peak / 32768.0f;  // 0..1

    if (state == State::RUNNING && lastMicLevel >= soundThreshold()) {
        stopRun();
    }
}

void TimerApp::render() {
    const int screenW = StickCP2.Display.width();
    const int screenH = StickCP2.Display.height();

    StickCP2.Display.startWrite();
    StickCP2.Display.setTextColor(WHITE, BLACK);

    // --- Строка статуса (состояние + порог звука) ---
    const char* stateText =
        (state == State::RUNNING) ? "RUN" :
        (state == State::PAUSED)  ? "PAUSE" : "STOP";

    String statusLine = String(stateText);
    if (soundLevel > 0) {
        statusLine += "  SND " + String(soundLevel) + "/" + String(kMaxLevel);
    } else {
        statusLine += "  SND OFF";
    }

    if (statusLine != drawnStatusLine) {
        StickCP2.Display.fillRect(0, kStatusLineY, screenW, kStatusLineH, BLACK);
        StickCP2.Display.setFont(&fonts::FreeSans9pt7b);
        StickCP2.Display.setTextDatum(top_center);
        StickCP2.Display.drawString(statusLine, screenW / 2, kStatusLineY);
        drawnStatusLine = statusLine;
    }

    // --- Строка с текущим уровнем звука и порогом срабатывания ---
    bool micLineVisible = soundLevel > 0;
    if (micLineVisible) {
        int micPct = (int)(lastMicLevel * 100.0f + 0.5f);
        int thrPct = (int)(soundThreshold() * 100.0f + 0.5f);
        String micLine = "MIC " + String(micPct) + "%  THR " + String(thrPct) + "%";

        if (micLine != drawnMicLine || !drawnMicLineVisible) {
            StickCP2.Display.fillRect(0, kMicLineY, screenW, kMicLineH, BLACK);
            StickCP2.Display.setFont(&fonts::FreeSans9pt7b);
            StickCP2.Display.setTextDatum(top_center);
            StickCP2.Display.drawString(micLine, screenW / 2, kMicLineY);
            drawnMicLine = micLine;
        }
    } else if (drawnMicLineVisible) {
        // Порог сбросили в 0 — строку больше не показываем, очищаем её место один раз.
        StickCP2.Display.fillRect(0, kMicLineY, screenW, kMicLineH, BLACK);
        drawnMicLine = "";
    }
    drawnMicLineVisible = micLineVisible;

    // --- Время по центру экрана ---
    String timeText = formatElapsed(elapsedMs());
    if (timeText != drawnTimeText) {
        int bandY = screenH / 2 + 10 - kTimeBandH / 2;
        StickCP2.Display.fillRect(0, bandY, screenW, kTimeBandH, BLACK);
        StickCP2.Display.setFont(&fonts::FreeSansBold18pt7b);
        StickCP2.Display.setTextDatum(middle_center);
        StickCP2.Display.drawString(timeText, screenW / 2, screenH / 2 + 10);
        drawnTimeText = timeText;
    }

    StickCP2.Display.endWrite();

    if (soundLevel != drawnSoundLevel) {
        GUI::progressBar((float)soundLevel / (float)kMaxLevel, 6);
        drawnSoundLevel = soundLevel;
    }
}

bool TimerApp::Loop() {
    handleButtons();
    if (exitRequested) {
        return false;
    }

    updateMic();
    render();

    return true;
}
