// **********************************************************************************
// ESP8266 Teleinfo configuration Include file
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
#ifndef __CONFIG_H__
#define __CONFIG_H__

// Include main project include file
#include "TICWifi.h"

#define CFG_SSID_SIZE     32  
#define CFG_PSK_SIZE      64
#define CFG_HOSTNAME_SIZE 16

#define CFG_HOST_SIZE     32
#define CFG_URL_SIZE      64

#define CFG_EMON_APIKEY_SIZE  32
#define CFG_EMON_DEFAULT_PORT 80
#define CFG_EMON_DEFAULT_HOST "emoncms.org"
#define CFG_EMON_DEFAULT_URL  "/input/post.json"

#define CFG_JDOM_APIKEY_SIZE  48
#define CFG_JDOM_ADCO_SIZE    12
#define CFG_JDOM_DEFAULT_PORT 80
#define CFG_JDOM_DEFAULT_HOST "jeedom.local"
#define CFG_JDOM_DEFAULT_URL  "/plugins/teleinfo/core/php/jeeTeleinfo.php"
#define CFG_JDOM_DEFAULT_ADCO "0000111122223333"

#define CFG_MQTT_TOPIC_SIZE    32
#define CFG_MQTT_DEFAULT_PORT  1883
#define CFG_MQTT_DEFAULT_HOST  "test.mosquitto.org"
#define CFG_MQTT_DEFAULT_TOPIC "Linky"

#pragma pack(push)  // push current alignment to stack
#pragma pack(1)     // set alignment to 1 byte boundary
typedef union _Config uConfig;

union _Config
{
  uint8_t    raweeprom [1024];  // raw data
  struct
  {
    uint16_t crc;                                       // CRC to garantee integrity of config struct
	// Config for Wifi
	char     Wifi_ssid      [CFG_SSID_SIZE + 1];        // SSID     
    char     Wifi_psk       [CFG_PSK_SIZE + 1];         // Pre shared key
    char     Wifi_host      [CFG_HOSTNAME_SIZE + 1];    // Hostname 
    char     Wifi_ap_psk    [CFG_PSK_SIZE + 1];         // Access Point Pre shared key
    // Config for emoncms
    char     emoncms_host   [CFG_HOST_SIZE + 1];        // FQDN 
    char     emoncms_apikey [CFG_EMON_APIKEY_SIZE + 1]; // Secret
    char     emoncms_url    [CFG_URL_SIZE + 1];         // Post URL
    uint16_t emoncms_port;                              // Protocol port (HTTP/HTTPS)
    uint8_t   emoncms_node;                              // optional node
    uint32_t emoncms_freq;                              // refresh rate
    // Config for jeedom
    char     jeedom_host    [CFG_HOST_SIZE + 1];        // FQDN 
    char     jeedom_apikey  [CFG_JDOM_APIKEY_SIZE + 1]; // Secret
    char     jeedom_url     [CFG_URL_SIZE + 1];         // Post URL
    char     jeedom_adco    [CFG_JDOM_ADCO_SIZE + 1];   // Identifiant compteur
    uint16_t jeedom_port;                               // Protocol port (HTTP/HTTPS)
    uint32_t jeedom_freq;                               // refresh rate
    // Config for MQTT
    char     mqtt_host      [CFG_HOST_SIZE + 1];        // URL 
    uint16_t mqtt_port;                                 // Protocol port (default 1883)
    char     mqtt_topic     [CFG_MQTT_TOPIC_SIZE + 1];  // MQTT Topic 
    uint32_t mqtt_freq;                                 // refresh rate
	char     mqtt_user      [33];
	char     mqtt_password  [33];
  };
};

#pragma pack(pop)

// Exported variables/object instancied in config.cpp
// ===================================================
extern uConfig config;
 
// Declared exported function from route.cpp
// ===================================================
bool readConfig (void);
bool saveConfig (void);

#endif 
