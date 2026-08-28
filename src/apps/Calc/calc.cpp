#include <M5StickCPlus2.h>
#include "calc.h"
#include "../../libs/gui/gui.h"
#include <vector>

static std::vector<char> data_array;

// keyboard data
static const byte KeyboarDataSize = 23;
static const char KeyboardData[KeyboarDataSize] = {
  '9', '8', '7', '6', '5', '4', '3', '2', '1', '0', '=', '<', '.', '!', '+', '-', '*', '/', '^', 'Q', 'M', 'R', 'E'
};
static int selectedIndex = 10;
static int lastIndex = selectedIndex;


// SCREEN
static void updateScreen(){
  StickCP2.Display.fillScreen(BLACK);
  String output = "";
  for (char c : data_array){
    output += c;
  }
  StickCP2.Display.setCursor(0, 0);
  StickCP2.Display.print(output);
  
  // Print input menu
  for (int y = 1; y <= linesPerPage; y++){
    StickCP2.Display.setCursor(charsPerLine, y);
    int indexOnY = selectedIndex - linesPerPage + y;
    if (indexOnY >= 0){
      StickCP2.Display.print(KeyboardData[indexOnY]);
    }
  }
}


// INPUT
static char inputKeyboard(){
  selectedIndex = updateMenuSelectionFast(selectedIndex, KeyboarDataSize);
  
  if (selectedIndex != lastIndex){
    updateScreen();
    lastIndex = selectedIndex;
    return KeyboardData[selectedIndex];
  }

  return 0;
}


// MAIN

CalcApp calcApp;

void CalcApp::Setup(){
  data_array.clear();
  selectedIndex = 10;
}


static char input;
bool CalcApp::Loop(){

  input = inputKeyboard();
  if (input != 0){
    data_array.push_back(input);
  }
  if (input == '<'){
    data_array.pop_back();
  }
  if (input == 'R'){
    Setup();
  }
  if (input == 'E'){
    return false;
  }

  return true;

}
