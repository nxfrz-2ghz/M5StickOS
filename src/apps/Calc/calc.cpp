#include "calc.h"
#include "../../libs/gui/gui.h"

namespace {
    const char FUNC_KEYS[13] = {
        '=', '<', '.', '!', '+', '-', '*', '/', '^', 'Q', 'M', 'R', 'E'
    };
    const char DIGIT_KEYS[13] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'S', 'C', 'T'
    };

    double fSqrt(double x) { return x < 0 ? NAN : sqrt(x); }
    double fSin (double x) { return sin(x); }
    double fCos (double x) { return cos(x); }
    double fTan (double x) { double t = tan(x); return fabs(t) > 1e9 ? NAN : t; }

    double fAdd(double a, double b) { return a + b; }
    double fSub(double a, double b) { return a - b; }
    double fMul(double a, double b) { return a * b; }
    double fDiv(double a, double b) { return b == 0 ? NAN : a / b; }
    double fPow(double a, double b) { return pow(a, b); }

    enum : uint8_t {
        K_UNARY  = 1 << 0,
        K_BINARY = 1 << 1,
        K_DEG    = 1 << 2,
    };

    struct KeyInfo {
        char key;
        uint8_t flags;
        double (*un)(double);
        double (*bin)(double, double);
    };

    const KeyInfo KEYS[] = {
        {'Q', K_UNARY,         fSqrt, nullptr},
        {'S', K_UNARY | K_DEG, fSin,  nullptr},
        {'C', K_UNARY | K_DEG, fCos,  nullptr},
        {'T', K_UNARY | K_DEG, fTan,  nullptr},
        {'+', K_BINARY, nullptr, fAdd},
        {'-', K_BINARY, nullptr, fSub},
        {'*', K_BINARY, nullptr, fMul},
        {'/', K_BINARY, nullptr, fDiv},
        {'^', K_BINARY, nullptr, fPow},
    };

    const KeyInfo* findKey(char k) {
        for (const auto &e : KEYS) if (e.key == k) return &e;
        return nullptr;
    }
}

void CalcApp::reset() {
    isSecond = false;
    showingResult = false;
    hasError = false;
    fisrtInp = "0";
    secondInp = "0";
    op = 0;
    result = 0;
}

String CalcApp::trimNumber(float value) {
    String s = String(value, 6);
    if (s.indexOf('.') != -1) {
        while (s.endsWith("0")) s.remove(s.length() - 1);
        if (s.endsWith(".")) s.remove(s.length() - 1);
    }
    if (s == "-0") s = "0";
    return s;
}


void CalcApp::applyKey(char key) {
    const KeyInfo* k = findKey(key);
    const uint8_t fl = k ? k->flags : 0;

    if (showingResult && key != 'R' && key != 'M') {
        if (fl & (K_UNARY | K_BINARY)) {
            fisrtInp = hasError ? "0" : trimNumber(result);
            secondInp = "0";
            op = (fl & K_BINARY) ? key : 0;
            isSecond = (fl & K_BINARY);
            showingResult = hasError = false;
            if (fl & K_BINARY) return;
        } else {
            reset();
        }
    }

    if (fl & K_UNARY) { applyUnary(key); return; }

    if (fl & K_BINARY) {
        if (secondInp != "0") { calculateResult(); showingResult = false; }
        op = key;
        isSecond = true;
        return;
    }

    String &cur = isSecond ? secondInp : fisrtInp;

    switch (key) {
        case 'R': reset(); break;
        case 'M': cur = trimNumber(result); break;
        case '=': if (op) calculateResult(); break;
        case '!':
            if (cur.startsWith("-")) cur.remove(0, 1);
            else if (cur != "0") cur = "-" + cur;
            break;
        case '.':
            if (cur.indexOf('.') == -1) cur += ".";
            break;
        case '<':
            if (cur.length() > 1) cur.remove(cur.length() - 1);
            else { cur = "0"; if (isSecond) { isSecond = false; op = 0; } }
            break;
        default:
            if (key >= '0' && key <= '9') {
                if (cur == "0") cur = String(key); else cur += key;
            }
    }
}

void CalcApp::applyUnary(char key) { // for unary
    const KeyInfo* k = findKey(key);
    if (!k || !k->un) return;
    String &cur = isSecond ? secondInp : fisrtInp;
    double x = cur.toDouble();
    if (k->flags & K_DEG) x *= DEG_TO_RAD;
    double v = k->un(x);
    if (isnan(v)) { hasError = true; showingResult = true; return; }
    cur = trimNumber((float)v);
}

void CalcApp::calculateResult() { // for binary
    const KeyInfo* k = findKey(op);
    double r = k->bin(fisrtInp.toDouble(), secondInp.toDouble());
    if (isnan(r)) hasError = true; else result = (float)r;
    showingResult = true;
}


void CalcApp::updateScreen() {
    String expr;
    if (showingResult) {
        expr = hasError ? "Error" : ("= " + trimNumber(result));
    } else {
        expr = fisrtInp;
        if (op) expr += " " + op; expr += " " + secondInp;
    }

    GUI::list(expr.c_str(), keyboardData, selectedIndex, false);

    // Индикатор pending справа снизу
    auto &d = StickCP2.Display;
    const int boxW = 50, boxH = 20;
    d.fillRect(d.width() - boxW, d.height() - boxH, boxW, boxH, TFT_BLACK);
    if (pending) {
        d.setFont(&fonts::FreeSans9pt7b);
        d.setTextColor(TFT_YELLOW, TFT_BLACK);
        String s = String("> ") + FUNC_KEYS[pendingIndex];
        int x = d.width() - d.textWidth(s) - 3;
        d.setCursor(x, d.height() - boxH + 1);
        d.print(s);
    }
}

bool CalcApp::runFunction(int index) {
    char key = FUNC_KEYS[index];
    if (key == 'E') return false;
    applyKey(key);
    return true;
}


bool CalcApp::Loop() {
    const int count = (int)keyboardData.size();
    bool redraw = false;

    // Циклический скролл c буфером в 2
    int newIndex = GUI::GUI::updateMenuSelection(selectedIndex + 1, count + 2, 150) - 1;
    if (newIndex < 0) newIndex = count - 1;
    else if (newIndex >= count) newIndex = 0;

    if (newIndex != selectedIndex) {
        // Прокрутили — сначала применяем отложенную функцию к старой строке
        if (pending) {
            pending = false;
            if (!runFunction(pendingIndex)) return false;
        }
        selectedIndex = newIndex;
        redraw = true;
    }

    if (StickCP2.BtnA.wasPressed()) {
        if (pending && pendingIndex == selectedIndex &&
            millis() - pendingTime < DOUBLE_PRESS_MS) {
            // Двойное нажатие -> цифра / sin / cos / tg из 2 колонки
            pending = false;
            applyKey(DIGIT_KEYS[selectedIndex]);
            redraw = true;
        } else {
            // Первое нажатие: если висело другое отложенное — применяем его
            if (pending) {
                pending = false;
                if (!runFunction(pendingIndex)) return false;
            }
            pending = true;
            pendingTime = millis();
            pendingIndex = selectedIndex;
            redraw = true;
        }
    }

    // Таймаут двойного нажатия -> одиночное нажатие, функция из 1 колонки
    if (pending && millis() - pendingTime >= DOUBLE_PRESS_MS) {
        pending = false;
        if (!runFunction(pendingIndex)) return false;
        redraw = true;
    }

    if (lastIndex == -1) redraw = true;

    if (redraw) {
        updateScreen();
        lastIndex = selectedIndex;
    }

    return true;
}