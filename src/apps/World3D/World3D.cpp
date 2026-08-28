#include <M5StickCPlus2.h>
#include "World3D.h"

struct rgbColor{
  int r;
  int g;
  int b;
};

typedef struct{
  int x;
  int y;
  uint16_t color;
  rgbColor rgb;
} obst;


struct isObs{
  bool status;
  obst infos;
};


struct hblock {
  int      distance;
  int      beginWall;
  int      beginGrass;
  uint16_t color; 
};


struct g_map{
  uint16_t W;
  uint16_t H;
  int mapW = 55;
  int mapH = 55;
  int directionDegree = 30;
  int maxDegreeLine   = 60;
  int maxDistanceLine = 20;
  int blockSize = 5;
  hblock historyBlocks[380];
  obst player = { 30, 5, 0, {0,0,0} };
  int nbObstacles = 0;
  obst* obstacles;
  bool _3DInit = false;
};

g_map game;

static obst mapObstacle[5] = {
  { 10, 10, 0, {0,0,0} },
  { 40, 10, 0, {0,0,0} },
  { 25, 25, 0, {0,0,0} },
  { 10, 40, 0, {0,0,0} },
  { 40, 40, 0, {0,0,0} }
};


static rgbColor __BLACK_RGB_COLOR__   = { 0,   0,   0 };
static rgbColor __WALL_RGB_COLOR__    = { 128, 128, 128 };
static rgbColor __SKY_RGB_COLOR__     = { 65,  105, 225 };
static rgbColor __GRASS_RGB_COLOR__   = { 85,  107, 47 };

static void addObstacleMap(){
  bool corner = false;
  for (int i = 1; i <= game.mapH && game.nbObstacles<=7000; i++){
      for (int j = 0; j <= game.mapW; j++){
        if (i == 1 || i == game.mapH || j == 0 || j ==  game.mapW){
              corner = ! ( ((j == 0 || j == game.mapW)  &&  i % 5) || j % 5 );
              game.obstacles[game.nbObstacles].x = j;
              game.obstacles[game.nbObstacles].y = i;
              game.obstacles[game.nbObstacles].color = corner ? TFT_DARKGREY : BLACK;
              game.obstacles[game.nbObstacles].rgb = corner == false ? __WALL_RGB_COLOR__ : __BLACK_RGB_COLOR__;
              game.nbObstacles++;
            }
    }
  }
  for (int _b = 0; _b<=4; _b++){
    for (int _h=0; _h<=game.blockSize; _h++){
      for (int _w=0; _w<=game.blockSize; _w++){
        corner = ((_h == 0  && _w == 0) || (_h == game.blockSize && _w == 0)  ||
                  (_h == 0  && _w == game.blockSize) ||  (_h == game.blockSize && _w == game.blockSize)); 
        game.obstacles[game.nbObstacles].x = mapObstacle[_b].x + _w;
        game.obstacles[game.nbObstacles].y = mapObstacle[_b].y + _h;
        game.obstacles[game.nbObstacles].color = corner ? BLACK : TFT_DARKGREY;
        game.obstacles[game.nbObstacles].rgb = corner == false ? __WALL_RGB_COLOR__ : __BLACK_RGB_COLOR__;
        game.nbObstacles++;   
      } 
    }
  }
}


static isObs isObstacle(int x, int y){
  obst  emptyObst;
  isObs res  = { false, emptyObst };
  for (int i = 0; i<=game.nbObstacles; i++){
    if ((x == game.obstacles[i].x && y == game.obstacles[i].y) ||
        x < 0 || x > game.mapW || y <= 0 || y >= game.mapH ){
        res.status = true;
        res.infos = game.obstacles[i];
        return res;
    }
  }
  return res;
}

static obst getCoord(int x, int y, int dist, float degree) {
  obst o;
  o.x = x + dist * cos(3.14 * degree / 180);
  o.y = y + dist * sin(3.14 * degree / 180);
  o.color = 0;
  o.rgb = __BLACK_RGB_COLOR__;
  return o;
}

static void drawLine(uint16_t color){
  int nx, ny;
  obst tmpCoord;
  isObs res;

  for (int degree = game.directionDegree; degree <= game.directionDegree + game.maxDegreeLine; degree++){
    for (int distance = 0; distance<=game.maxDistanceLine; distance++){
        tmpCoord =  getCoord(game.player.x, game.player.y, distance, degree);
        res = isObstacle(tmpCoord.x, tmpCoord.y);
        if (res.status == true) {
          continue;
        }
        StickCP2.Display.drawPixel(tmpCoord.x, tmpCoord.y, color);
     }    
  }
}

static uint16_t getColorFromDistance(rgbColor _rgbcolor, int distance, int maxDistance, int coef) {

  int r,g,b, ratio = ( distance) * coef / maxDistance;
  uint16_t _r, _g, _b;

  if ((_rgbcolor.r == 0 && _rgbcolor.b == 0 && _rgbcolor.g == 0) == true){
    r = _rgbcolor.r;
    g = _rgbcolor.g;
    b = _rgbcolor.b;
  }
  else {
    r = _rgbcolor.r - (_rgbcolor.r - ratio > 0 ? ratio : 0);
    g = _rgbcolor.g - (_rgbcolor.g - ratio > 0 ? ratio : 0);
    b = _rgbcolor.b - (_rgbcolor.b - ratio > 0 ? ratio : 0);
  }
  
  _r = ((r >> 3) & 0x1f) << 11;
  _g = ((g >> 2) & 0x3f) << 5;
  _b = (b >> 3)  & 0x1f;

    return (uint16_t) (_r | _g | _b);
};

static void draw3DLine(){
  int nx, ny;
  obst tmpCoord;
  isObs res;
  int wallHeightPercent, wallHeightPixel, wallBeginPixel;
  int countRay    = 1; 
  int rayWidth    = game.W  / game.maxDegreeLine;
  int _angle, distance, _distance;


  for (int degree =  game.maxDegreeLine; degree >= 0 ; degree--){
    countRay++;
    wallHeightPixel = 0;
    wallBeginPixel  = game.H / 2;
    for (distance = 0; distance<=game.maxDistanceLine; distance++){
        tmpCoord =  getCoord(game.player.x, game.player.y, distance, game.directionDegree + degree );
        res = isObstacle(tmpCoord.x, tmpCoord.y);
        _distance = distance;

        if (res.status == true) {
          _angle =  ((game.directionDegree + game.maxDistanceLine / 2) * 3.14 / 180) - (game.directionDegree + degree)  * 3.14 / 180;
					_distance = distance * cos(_angle);
          wallHeightPercent = ((game.maxDistanceLine - _distance) * 100) / game.maxDistanceLine;
          wallHeightPixel   = wallHeightPercent * game.H  / 100;
          wallBeginPixel    = (game.H -  wallHeightPixel) / 2;
          // WALL
          for (int h = wallBeginPixel;h <= wallBeginPixel + wallHeightPixel; h++){
            for (int w = 0; w <= rayWidth;w++ )  {
              res.infos.color = getColorFromDistance(res.infos.rgb, distance, game.maxDistanceLine, 100);              
              StickCP2.Display.drawPixel((countRay * rayWidth) + w, h,  res.infos.color);
              // res.infos.color == BLACK ? BLACK : w % 3 == 1 && h % 2 == 0 || w % 3 == 0 && h % 2 == 1? TFT_LIGHTGREY: TFT_DARKGREY);
            }
          }
          break;
        }
     }
    if (game._3DInit == true && 
      game.historyBlocks[countRay].distance == _distance &&
      game.historyBlocks[countRay].color == res.infos.color)  {
      continue;
    }
    // SKY
    for (int h = 0; h <= wallBeginPixel; h++){
      for (int w = 0; w <= rayWidth; w++ )  {
        if (game._3DInit == true && h < game.historyBlocks[countRay].beginWall) 
          continue;
        StickCP2.Display.drawPixel((countRay * rayWidth) + w, h, getColorFromDistance(__SKY_RGB_COLOR__, wallBeginPixel - h, wallBeginPixel, 50) );
      }
    }
    // GRASS
    for (int h = wallBeginPixel + wallHeightPixel; h <= game.H; h++){
      for (int w = 0; w < rayWidth;w++ )  {
        if (game._3DInit == true 
            && wallBeginPixel + wallHeightPixel >= game.historyBlocks[countRay].beginGrass 
            && h > game.historyBlocks[countRay].beginGrass) 
            continue;
        StickCP2.Display.drawPixel((countRay * rayWidth) + w, h,  getColorFromDistance(__GRASS_RGB_COLOR__, game.H - h, game.H, 50));
      }
    }   
    game.historyBlocks[countRay].distance = _distance;
    game.historyBlocks[countRay].beginWall = wallBeginPixel;
    game.historyBlocks[countRay].beginGrass = wallBeginPixel + wallHeightPixel;
    game.historyBlocks[countRay].color = res.infos.color;
  }
  game._3DInit = true;
}



World3DApp world3DApp;

void World3DApp::Setup()
{
  // На случай повторного запуска приложения освобождаем буфер
  // от предыдущего запуска и сбрасываем счётчик препятствий,
  // иначе при каждом входе в приложение будет утечка памяти.
  if (game.obstacles) {
    free(game.obstacles);
    game.obstacles = nullptr;
  }
  game.nbObstacles = 0;
  game._3DInit = false;

  game.obstacles = (obst*)malloc(3000 * sizeof(obst));
  StickCP2.Display.fillScreen(TFT_BLACK);
  game.W = StickCP2.Display.width();
  game.H = StickCP2.Display.height();
  addObstacleMap();
  draw3DLine();
}

void World3DApp::Exit()
{
  if (game.obstacles) {
    free(game.obstacles);
    game.obstacles = nullptr;
  }
}



bool World3DApp::Loop()
{
  obst tmpCoord;
  isObs res;
  bool leftButton  = StickCP2.BtnPWR.isPressed();
  bool rightButton = StickCP2.BtnB.isPressed();
  bool frontButton = StickCP2.BtnA.isPressed();
  bool backButton  = leftButton && rightButton;
  bool userAction  = leftButton || rightButton || frontButton;
  
  for (int i = 0; i<=game.nbObstacles; i++){
     StickCP2.Display.drawPixel(game.obstacles[i].x, game.obstacles[i].y, TFT_RED);//game.obstacles[i].color);
  }
  if (userAction){
    StickCP2.Display.drawPixel(game.player.x, game.player.y, BLACK);
    drawLine(BLACK);
    if (backButton) {
      return false;
    } 
    else if (frontButton) {
      tmpCoord =  getCoord(game.player.x, game.player.y, 2, game.directionDegree + ( game.maxDegreeLine / 2) );
      game.player.x = tmpCoord.x;
      game.player.y = tmpCoord.y;
    }
    else if (leftButton) {
      game.directionDegree=game.directionDegree+10;
    }
    else  if (rightButton) {
      game.directionDegree=game.directionDegree-10;
    }
    draw3DLine();
    StickCP2.Display.drawPixel(game.player.x, game.player.y, RED);
    drawLine(GREEN);
   }
   delay(100);
   return true;
}