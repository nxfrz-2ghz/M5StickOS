#include "calc.h"
#include "../../libs/gui/gui.h"

// keyboard data
static const byte KeyboarDataSize = 23;
static const char KeyboardData[KeyboarDataSize] = {
  '9', '8', '7', '6', '5', '4', '3', '2', '1', '0', '=', '<', '.', '!', '+', '-', '*', '/', '^', 'Q', 'M', 'R', 'E'
};

// SCREEN
void CalcApp::updateScreen(){
  StickCP2.Display.fillScreen(BLACK);
  String output = "";
  for (char c : data_array){
    output += c;
  }
  StickCP2.Display.setCursor(0, 0);
  StickCP2.Display.print(output);

  // Print input menu
  for (int y = 1; y <= displayTextLinesPerPage; y++){
    StickCP2.Display.setCursor(displayTextCharsPerLine, y);
    int indexOnY = selectedIndex - displayTextLinesPerPage + y;
    if (indexOnY >= 0){
      StickCP2.Display.print(KeyboardData[indexOnY]);
    }
  }
}


// INPUT
char CalcApp::inputKeyboard(){
  selectedIndex = updateMenuSelection(selectedIndex, KeyboarDataSize);

  if (selectedIndex != lastIndex){
    updateScreen();
    lastIndex = selectedIndex;
    return KeyboardData[selectedIndex];
  }

  return 0;
}


// MAIN

bool CalcApp::Loop(){
  char input = inputKeyboard();
  if (input != 0){
    data_array.push_back(input);
  }
  if (input == '<' && !data_array.empty()){
    data_array.pop_back();
  }
  if (input == 'R'){
    data_array.clear();
    selectedIndex = 10;
    lastIndex = 10;
    updateScreen();
  }
  if (input == 'E'){
    return false;
  }

  return true;
}
