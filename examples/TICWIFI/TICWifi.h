// **********************************************************************************
// ESP8266 WifInfo WEB Server global Include file
// **********************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CC-BY-SA license:
// Attribution-NonCommercial-ShareAlike 4.0 International License
// http://creativecommons.org/licenses/by-nc-sa/4.0/
//
// For any explanation about teleinfo or use, see my blog
// http://hallard.me/category/tinfo
//
// This program works with the Wifinfo board
// see schematic here https://github.com/hallard/teleinfo/tree/master/Wifinfo
//
// Written by Charles-Henri Hallard (http://hallard.me)
// Modified by Arly - only for TIC Standard mode, only onboard leds, no debug at all, no OTA 
//  use of static array for TIC labels (no malloc/free)
//
// History : V2 2019-02-24
//
// All text above must be included in any redistribution.
//
// **********************************************************************************
#ifndef TICWIfI_H
#define TICWIfI_H

// Include Arduino header
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <FS.h>

extern "C" {
#include "user_interface.h"
}
#include "LibTeleinfoStd.h"
#include "webserver.h"
#include "webclient.h"
#include "config.h"

#define WIFINFO_VERSION "2.0.1"
#define BLINK_LED_MS    100 // 100 ms blink
#define ESP_LED_PIN     2 

// sysinfo informations
typedef struct 
{
  String  sys_uptime;
  String  TICDate;
  uint8_t LastError;
  uint32_t jeedom_POSTret; // returned code of last POST request
} _sysinfo;

// Exported variables/object instancied in main sketch
// ===================================================
extern TInfo tinfo;
extern _sysinfo sysinfo;
extern Ticker Tick_emoncms;
extern Ticker Tick_jeedom;
extern Ticker Tick_mqtt;

// Exported function located in main sketch
// ===================================================
void ResetConfig(void);
void UpdateSysinfo (void);
void Task_emoncms();
void Task_jeedom();
void Task_mqtt();

#endif
