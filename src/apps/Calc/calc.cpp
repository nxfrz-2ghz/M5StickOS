#include "calc.h"
#include "../../libs/gui/gui.h"

namespace {
    const char FUNC_KEYS[13] = {
        '=', '<', '.', '!', '+', '-', '*', '/', '^', 'Q', 'M', 'R', 'E'
    };
    const char DIGIT_KEYS[13] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'S', 'C', 'T'
    };
}

void CalcApp::reset() {
    isSecond = false;
    showingResult = false;
    hasError = false;
    fisrtInp = "0";
    secondInp = "0";
    op = "";
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

void CalcApp::calculateResult() {
    float a = fisrtInp.toDouble();
    float b = secondInp.toDouble();
    hasError = false;

    if (op == "+") result = a + b;
    else if (op == "-") result = a - b;
    else if (op == "*") result = a * b;
    else if (op == "/") {
        if (b == 0) hasError = true;
        else result = a / b;
    }
    else if (op == "^") result = pow(a, b);

    showingResult = true;
}

void CalcApp::applyUnary(char key) {
    String &cur = isSecond ? secondInp : fisrtInp;
    double rad = cur.toDouble() * DEG_TO_RAD;
    double v = 0;

    if (key == 'S') {
        v = sin(rad);
    } else if (key == 'C') {
        v = cos(rad);
    } else { // 'T'
        double c = cos(rad);
        if (fabs(c) < 1e-9) {
            hasError = true;
            showingResult = true;
            return;
        }
        v = sin(rad) / c;
    }

    cur = trimNumber((float)v);
}

void CalcApp::applyKey(char key) {
    // После показа результата любая цифра/оператор начинают новый ввод
    if (showingResult && key != 'R' && key != 'M') {
        bool isOperator = (key=='+' || key=='-' || key=='*' || key=='/' || key=='^' || key=='Q');
        bool isUnary = (key=='S' || key=='C' || key=='T');

        if (isOperator) {
            fisrtInp = hasError ? "0" : trimNumber(result);
            secondInp = "0";
            op = String(key);
            isSecond = true;
            showingResult = false;
            hasError = false;
            return;
        } else if (isUnary) {
            // sin/cos/tg применяются к результату
            fisrtInp = hasError ? "0" : trimNumber(result);
            secondInp = "0";
            op = "";
            isSecond = false;
            showingResult = false;
            hasError = false;
            // и продолжаем обработку ниже
        } else {
            reset(); // и продолжаем обработку ниже как обычный ввод
        }
    }

    switch (key) {
        case 'R':
            reset();
            break;

        case 'M': {
            String val = trimNumber(result);
            if (!isSecond) fisrtInp = val; else secondInp = val;
            break;
        }

        case '!': {
            String &cur = isSecond ? secondInp : fisrtInp;
            if (cur.startsWith("-")) cur.remove(0, 1);
            else if (cur != "0") cur = "-" + cur;
            break;
        }

        case '<': {
            String &cur = isSecond ? secondInp : fisrtInp;
            if (cur.length() > 1) {
                cur.remove(cur.length() - 1);
            } else {
                cur = "0";
                // Если второе число пустое — стираем знак
                if (isSecond) { isSecond = false; op = ""; }
            }
            break;
        }

        case '.': {
            String &cur = isSecond ? secondInp : fisrtInp;
            if (cur.indexOf('.') == -1) cur += ".";
            break;
        }

        case '=':
            if (op != "") calculateResult();
            break;

        case 'S': case 'C': case 'T':
            applyUnary(key);
            break;

        case '+': case '-': case '*': case '/': case '^': case 'Q':
            // Если второе число введено — выполняем = и добавляем знак
            if (secondInp != "0") {
                calculateResult();
                showingResult = false;
            }
            op = String(key);
            isSecond = true;
            break;

        default: // цифры 0-9
            if (key >= '0' && key <= '9') {
                String &cur = isSecond ? secondInp : fisrtInp;
                if (cur == "0") cur = String(key);
                else cur += key;
            }
            break;
    }
}

void CalcApp::updateScreen() {
    String expr;
    if (showingResult) {
        expr = hasError ? "Error" : ("= " + trimNumber(result));
    } else {
        expr = fisrtInp;
        if (op != "") expr += " " + op + " " + secondInp;
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