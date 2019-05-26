// **********************************************************************************
// ESP8266 Teleinfo WEB Client Include file
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
// !! Todo : function Emoncms to complete as only numeric data are sent,
//    no informations about Tarif/Contrat and other alphanumeric data !!

#ifndef WEBCLIENT_H
#define WEBCLIENT_H

// Include main project include file
#include "TICWifi.h"

// Exported variables/object instancied in main sketch
// ===================================================

// declared exported function from route.cpp
// ===================================================
boolean httpPost_   (char* host, uint16_t port, char* url, String payload);
boolean httpGet     (char* host, uint16_t port, char* url);
boolean emoncmsPost (void);
boolean jeedomPost  (void);
boolean mqttPublish (void);

#endif