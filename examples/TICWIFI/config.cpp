// **********************************************************************************
// ESP8266 Teleinfo configuration functions
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
#include "config.h" 

// Configuration structure for whole program
uConfig config;

/* ======================================================================
Function: crc16Update
Purpose : calculate a 16 bit polynomial CRC
Input   : seed (previous crc result)
          data to be crc'ed
Output  : result of calcul
Comments: -
====================================================================== */
uint16_t crc16Update (uint16_t crc, uint8_t a)
{
  uint8_t i;
  
  crc ^= a;
  for (i = 0; i < 8; i++)
  {
    if (crc & 1)
      crc = (crc >> 1) ^ 0xA001;
    else
      crc = (crc >> 1);
  }
  return crc;
}

/* ======================================================================
Function: ResetConfig
Purpose : Set configuration to default values
Input   : -
Output  : -
Comments: -
====================================================================== */
void ResetConfig(void) 
{
  // Start cleaning all that stuff
  memset(config.raweeprom, 0, 1024);

  // Set default Hostname
  sprintf_P(config.Wifi_host, PSTR("TICWIFI-%06X"), ESP.getChipId());

  // Emoncms
  strcpy_P(config.emoncms_host, CFG_EMON_DEFAULT_HOST);
  config.emoncms_port = CFG_EMON_DEFAULT_PORT;
  strcpy_P(config.emoncms_url, CFG_EMON_DEFAULT_URL);

  // Jeedom
  strcpy_P(config.jeedom_host, CFG_JDOM_DEFAULT_HOST);
  config.jeedom_port = CFG_JDOM_DEFAULT_PORT;
  strcpy_P(config.jeedom_url, CFG_JDOM_DEFAULT_URL);
  
  //MQTT
  strcpy_P(config.mqtt_host, CFG_MQTT_DEFAULT_HOST);
  config.mqtt_port = CFG_MQTT_DEFAULT_PORT;
  strcpy_P(config.mqtt_topic, CFG_MQTT_DEFAULT_TOPIC);
  
  // save back
  saveConfig();
}

/* ======================================================================
Function: readConfig
Purpose : fill config structure with data located into eeprom
Input   : true if we need to clear actual struc in case of error
Output  : true if config found and crc ok, false otherwise
Comments: -
====================================================================== */
bool readConfig (void) 
{
  uint16_t crc;
  uint16_t i;
  uint8_t data;
 
  // Our configuration is stored into EEPROM
  EEPROM.begin (1024);

  // Init CRC
  crc = ~0;

  // For whole size of config structure
  for (i = 0; i <1024; i++)
  {
    // read data
    data = EEPROM.read(i);
    // store to config struct
	  config.raweeprom [i] = data;
    // calc CRC
    if (i > 1) 
    {
      crc = crc16Update(crc, data);
    }
  }
    
  // CRC Error ?
  if (crc != config.crc) 
    return false;
  
  return true;
}

/* ======================================================================
Function: saveConfig
Purpose : save config structure values into eeprom
Input   : -
Output  : true if saved and readback ok
Comments: once saved, config is read again to check the CRC
====================================================================== */
bool saveConfig (void) 
{
  uint16_t i;
  bool ret_code;


  // Init CRC
  config.crc = ~0;
  for (i = 2; i < 1024; i++)
  {
    // For whole size of config structure appart crc itself, calculate CRC
    config.crc = crc16Update (config.crc, config.raweeprom [i]);
  }

  // Our configuration is stored into EEPROM
  EEPROM.begin (1024);
  for (i = 0; i < 1024; i++)
  {
    EEPROM.write (i, config.raweeprom [i]);
  }
  // Physically save
  ret_code = EEPROM.commit();
  
  // Read Again to see if saved ok, but do not clear if error 
  // this avoid clearing default config
  //ret_code = readConfig ();

  return ret_code;
}
