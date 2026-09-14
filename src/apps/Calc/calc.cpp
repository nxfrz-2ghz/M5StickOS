#include "calc.h"
#include "../../libs/gui/gui.h"

void CalcApp::reset() {
    numnum = false;
    showingResult = false;
    hasError = false;
    firstinp = "0";
    secondinp = "0";
    op = "";
    result = 0;
}

String CalcApp::trimNumber(float value) {
    String s = String(value, 6);
    if (s.indexOf('.') != -1) {
        while (s.endsWith("0")) s.remove(s.length() - 1);
        if (s.endsWith(".")) s.remove(s.length() - 1);
    }
    return s;
}

void CalcApp::calculateResult() {
    float a = firstinp.toFloat();
    float b = secondinp.toFloat();
    hasError = false;

    if (op == "+") result = a + b;
    else if (op == "-") result = a - b;
    else if (op == "*") result = a * b;
    else if (op == "/") {
        if (b == 0) hasError = true;
        else result = a / b;
    }
    else if (op == "^") result = pow(a, b);
    else if (op == "Q") result = sqrt(a);

    showingResult = true;
}

void CalcApp::applyKey(char key) {
    // После показа результата любая цифра/оператор начинают новый ввод
    if (showingResult && key != 'R' && key != 'M') {
        bool isOperator = (key=='+' || key=='-' || key=='*' || key=='/' || key=='^' || key=='Q');
        if (isOperator) {
            firstinp = hasError ? "0" : trimNumber(result);
            secondinp = "0";
            op = String(key);
            numnum = true;
            showingResult = false;
            hasError = false;
            return;
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
            if (!numnum) firstinp = val; else secondinp = val;
            break;
        }

        case '!': {
            String &cur = numnum ? secondinp : firstinp;
            if (cur.startsWith("-")) cur.remove(0, 1);
            else if (cur != "0") cur = "-" + cur;
            break;
        }

        case '<': {
            String &cur = numnum ? secondinp : firstinp;
            if (cur.length() > 1) cur.remove(cur.length() - 1);
            else cur = "0";
            break;
        }

        case '.': {
            String &cur = numnum ? secondinp : firstinp;
            if (cur.indexOf('.') == -1) cur += ".";
            break;
        }

        case '=':
            if (op != "") calculateResult();
            break;

        case '+': case '-': case '*': case '/': case '^': case 'Q':
            op = String(key);
            numnum = true;
            break;

        default: // цифры 0-9
            if (key >= '0' && key <= '9') {
                String &cur = numnum ? secondinp : firstinp;
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
        expr = firstinp;
        if (op != "") expr += " " + op + " " + secondinp;
    }

    displayList(expr.c_str(), keyboardData, selectedIndex, false);
}

bool CalcApp::Loop() {
    int newIndex = updateMenuSelection(selectedIndex, (int)keyboardData.size(), 150);
    bool redraw = false;

    if (newIndex != selectedIndex) {
        selectedIndex = newIndex;
        redraw = true;
    }

    if (StickCP2.BtnA.wasPressed()) {
        char key = keyboardData[selectedIndex][0];

        if (key == 'E') {
            return false;
        }

        applyKey(key);
        redraw = true;
    }

    if (lastIndex == -1) redraw = true; // первая отрисовка

    if (redraw) {
        updateScreen();
        lastIndex = selectedIndex;
    }

    return true;
}