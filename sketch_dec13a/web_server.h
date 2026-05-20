#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "M5StickCPlus2.h"
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "gui.h"

void initWebServer();
void loopWebServer();

#endif
