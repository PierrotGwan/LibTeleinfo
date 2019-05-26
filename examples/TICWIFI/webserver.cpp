// **********************************************************************************
// ESP8266 Teleinfo WEB Server functions
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

// Include Arduino header
#include "webserver.h"

// Optimize string space in flash, avoid duplication
const char FP_JSON_START[] PROGMEM = "{\r\n";
const char FP_JSON_END[]   PROGMEM = "\r\n}\r\n";
const char FP_QCQ[]        PROGMEM = "\":\"";
const char FP_QCNL[]       PROGMEM = "\",\r\n\"";
const char FP_RESTART[]    PROGMEM = "OK, Redémarrage en cours\r\n";
const char FP_NL[]         PROGMEM = "\r\n";

ESP8266WebServer server;

/* ======================================================================
Function: formatSize
Purpose : format a asize to human readable format
Input   : size
Output  : formated string
Comments: -
====================================================================== */
String formatSize (size_t bytes)
{
  if (bytes < 1024)
  {
    return String (bytes) + F (" byte");
  }
  else if (bytes < (1024 * 1024) )
  {
    return String (bytes / 1024.0) + F (" KB");
  }
  else if (bytes < (1024 * 1024 * 1024) )
  {
    return String (bytes / 1024.0 / 1024.0) +
           F (" MB");
  }
  else
  {
    return String (bytes / 1024.0 / 1024.0 / 1024.0) +
           F (" GB");
  }
}

/* ======================================================================
Function: getContentType
Purpose : return correct mime content type depending on file extension
Input   : -
Output  : Mime content type
Comments: -
====================================================================== */
String getContentType (String filename)
{
  if (filename.endsWith (".htm") )
  {
    return F ("text/html");
  }
  else if (filename.endsWith (".html") )
  {
    return F ("text/html");
  }
  else if (filename.endsWith (".css") )
  {
    return F ("text/css");
  }
  else if (filename.endsWith (".json") )
  {
    return F ("text/json");
  }
  else if (filename.endsWith (".js") )
  {
    return F ("application/javascript");
  }
  else if (filename.endsWith (".png") )
  {
    return F ("image/png");
  }
  else if (filename.endsWith (".gif") )
  {
    return F ("image/gif");
  }
  else if (filename.endsWith (".jpg") )
  {
    return F ("image/jpeg");
  }
  else if (filename.endsWith (".ico") )
  {
    return F ("image/x-icon");
  }
  else if (filename.endsWith (".xml") )
  {
    return F ("text/xml");
  }
  else if (filename.endsWith (".pdf") )
  {
    return F ("application/x-pdf");
  }
  else if (filename.endsWith (".zip") )
  {
    return F ("application/x-zip");
  }
  else if (filename.endsWith (".gz") )
  {
    return F ("application/x-gzip");
  }
  else if (filename.endsWith (".otf") )
  {
    return F ("application/x-font-opentype");
  }
  else if (filename.endsWith (".eot") )
  {
    return F ("application/vnd.ms-fontobject");
  }
  else if (filename.endsWith (".svg") )
  {
    return F ("image/svg+xml");
  }
  else if (filename.endsWith (".woff") )
  {
    return F ("application/x-font-woff");
  }
  else if (filename.endsWith (".woff2") )
  {
    return F ("application/x-font-woff2");
  }
  else if (filename.endsWith (".ttf") )
  {
    return F ("application/x-font-ttf");
  }
  return "text/plain";
}

/* ======================================================================
Function: handleFileRead
Purpose : return content of a file stored on SPIFFS file system
Input   : file path
Output  : true if file found and sent
Comments: -
====================================================================== */
bool handleFileRead (String path)
{
  File file;
  size_t sent;
  String contentType;
  String pathWithGz;

  contentType = getContentType (path);
  pathWithGz = path + ".gz";

  if (SPIFFS.exists (pathWithGz)
      || SPIFFS.exists (path) )
  {
    if (SPIFFS.exists (pathWithGz) )
    {
      path += ".gz";
    }

    file = SPIFFS.open (path, "r");
    sent = server.streamFile (file, contentType);
    file.close();
    return true;
  }
  //  server.send(404, "text/plain", "File Not Found");
  return false;
}

/* ======================================================================
Function: handleFormConfig
Purpose : handle main configuration page
Input   : -
Output  : -
Comments: -
====================================================================== */
void handleFormConfig (void)
{
  String response = "";
  int ret ;
  int itemp;

  // We validated config ?
  if (server.hasArg ("save") )
  {

    // TICWifi
    strncpy (config.Wifi_ssid,
             server.arg ("ssid").c_str(),     CFG_SSID_SIZE);
    strncpy (config.Wifi_psk,
             server.arg ("psk").c_str(),      CFG_PSK_SIZE);
    strncpy (config.Wifi_host,
             server.arg ("host").c_str(),
             CFG_HOSTNAME_SIZE);
    strncpy (config.Wifi_ap_psk,
             server.arg ("ap_psk").c_str(),   CFG_PSK_SIZE);

    // Emoncms
    strncpy (config.emoncms_host,
             server.arg ("emon_host").c_str(),  CFG_HOST_SIZE);
    strncpy (config.emoncms_url,
             server.arg ("emon_url").c_str(),   CFG_URL_SIZE);
    strncpy (config.emoncms_apikey,
             server.arg ("emon_apikey").c_str(),
             CFG_EMON_APIKEY_SIZE);
    itemp = server.arg ("emon_node").toInt();
    config.emoncms_node = (itemp >= 0
                           && itemp <= 255) ? itemp : 0;
    itemp = server.arg ("emon_port").toInt();
    config.emoncms_port = (itemp >= 0
                           && itemp <= 65535) ? itemp :
                          CFG_EMON_DEFAULT_PORT;
    itemp = server.arg ("emon_freq").toInt();
    if (itemp > 0 && itemp <= 86400)
    {
      // Emoncms Ticker Update if needed
      Tick_emoncms.detach ();
      Tick_emoncms.attach (itemp, Task_emoncms);
    }
    else
    {
      itemp = 0;
    }
    config.emoncms_freq = itemp;

    // jeedom
    strncpy (config.jeedom_host,
             server.arg ("jdom_host").c_str(),
             CFG_HOST_SIZE + 1);
    strncpy (config.jeedom_url,
             server.arg ("jdom_url").c_str(),
             CFG_URL_SIZE + 1);
    strncpy (config.jeedom_apikey,
             server.arg ("jdom_apikey").c_str(),
             CFG_JDOM_APIKEY_SIZE + 1);
    strncpy (config.jeedom_adco,
             server.arg ("jdom_adco").c_str(),
             CFG_JDOM_ADCO_SIZE  + 1);
    itemp = server.arg ("jdom_port").toInt();
    config.jeedom_port = (itemp >= 0
                          && itemp <= 65535) ? itemp :
                         CFG_JDOM_DEFAULT_PORT;
    itemp = server.arg ("jdom_freq").toInt();
    if (itemp > 0 && itemp <= 86400)
    {
      // jeedom ticker Update if needed
      Tick_jeedom.detach();
      Tick_jeedom.attach (itemp, Task_jeedom);
    }
    else
    {
      itemp = 0 ;
    }
    config.jeedom_freq = itemp;

    // MQTT
    strncpy (config.mqtt_host,
             server.arg ("mqtt_url").c_str(),
             CFG_HOST_SIZE + 1);
    strncpy (config.mqtt_user,
             server.arg ("mqtt_user").c_str(),  33);
    strncpy (config.mqtt_password,
             server.arg ("mqtt_pwd").c_str(),   33);
    strncpy (config.mqtt_topic,
             server.arg ("mqtt_topic").c_str(),
             CFG_MQTT_TOPIC_SIZE  + 1);
    itemp = server.arg ("mqtt_port").toInt();
    config.mqtt_port = (itemp >= 0
                        && itemp <= 65535) ? itemp :
                       CFG_MQTT_DEFAULT_PORT;
    itemp = server.arg ("mqtt_freq").toInt();
    if (itemp > 0 && itemp <= 86400)
    {
      // MQTT ticker Update if needed
      Tick_mqtt.detach();
      Tick_mqtt.attach (itemp, Task_mqtt);
    }
    else
    {
      itemp = 0 ;
    }
    config.mqtt_freq = itemp;

    if (saveConfig () )
    {
      ret = 200;
      response = "OK";
    }
    else
    {
      ret = 500;
      response = "Unable to save configuration";
    }
  }
  else
  {
    ret = 400;
    response = "Missing Form field";
  }

  server.send (ret, "text/plain", response);
}

/* ======================================================================
Function: handleRoot
Purpose : handle main page /
Input   : -
Output  : -
Comments: -
====================================================================== */
void handleRoot (void)
{
  handleFileRead ("/index.htm");
}

/* ======================================================================
Function: formatNumberJSON
Purpose : check if data value is full number and send correct JSON format
Input   : String where to add response
          char * value to check
Output  : -
Comments: 00150 => 150
          ADSC  => "ADSC"
          1     => 1
====================================================================== */
void formatNumberJSON (String& response,
                       char* value)
{
  char* p;
  // we have at least something ?
  if (value && strlen (value) )
  {
    boolean isNumber = true;
    p = value;

    // just to be sure
    if (strlen (p) <= 16)
    {
      // check if value is number
      while (*p && isNumber)
      {
        if ( *p < '0' || *p > '9' )
        {
          isNumber = false;
        }
        p++;
      }

      // this will add "" on not number values
      if (!isNumber)
      {
        response += '\"' ;
        response += value ;
        response += F ("\"");
      }
      else
      {
        // this will remove leading zero on numbers
        p = value;
        while (*p == '0' && * (p + 1) )
        {
          p++;
        }
        response += p ;
      }
    }
  }
}

/* ======================================================================
Function: tinfoJSONTable
Purpose : dump all teleinfo values in JSON table format for browser
Input   : -
Output  : -
Comments: -
====================================================================== */
void tinfoJSONTable (void)
{
  uint8_t index;
  String response = "";
  sValueList labelitem;
  boolean first_item = true;

  // Got at least one ?
  index = tinfo.getIndexNextItem (
            0); // search for 1st item
  if (index > 0)
  {
    // Json start
    response += F ("[\r\n");
    // Loop thru the node
    while (tinfo.GetItem (index, &labelitem) == true)
    {
      // reset soft Watchdog to avoid ESP restart as this loop can be long
      ESP.wdtFeed();
      // First item do not add , separator
      if (first_item)
      {
        first_item = false;
      }
      else
      {
        response += F (",\r\n");
      }

      response += F ("{\"na\":\"");
      response += labelitem.name;
      response += F ("\", \"va\":");
      formatNumberJSON (response, labelitem.value);
      if (labelitem.horodate.DST[0] !=
          '\0') // must be not 0 or there is nothing in horodate
      {
        response += F (", \"ho\":\"");
        response += F ("20");
        response += labelitem.horodate.Year[0];
        response += labelitem.horodate.Year[1];
        response += "/";
        response += labelitem.horodate.Month[0];
        response += labelitem.horodate.Month[1];
        response += "/";
        response += labelitem.horodate.Day[0];
        response += labelitem.horodate.Day[1];
        response += " ";
        response += labelitem.horodate.Hour[0];
        response += labelitem.horodate.Hour[1];
        response += ":";
        response += labelitem.horodate.Min[0];
        response += labelitem.horodate.Min[1];
        response += ":";
        response += labelitem.horodate.Sec[0];
        response += labelitem.horodate.Sec[1];
        response += F ("\"");
      }
      response += F (", \"fl\":");
      response += labelitem.flags;
      response += F ("}");
      // go to next TIC item
      index = tinfo.getIndexNextItem (index);
    }
    // Json end
    response += F ("\r\n]");
    server.send ( 200, "text/json", response );
  }
  else
  {
    server.send ( 404, "text/plain", "No data" );
  }
}

/* ======================================================================
Function: sendJSON
Purpose : dump all valid TIC items in JSON
Input   : -
Output  : -
Comments: -
====================================================================== */
void sendJSON (void)
{
  uint8_t index;
  boolean first_item = true;
  String response = "";
  sValueList labelitem;

  UpdateSysinfo ();
  // Json start
  response += FPSTR (FP_JSON_START);
  response += F ("\"UPTIME\":");
  response += sysinfo.sys_uptime;
  response += F ("\",\r\n\"TICDate\":\"");
  response += sysinfo.TICDate;
  response += F (",\r\n\"TICLastError\":\"");
  response += sysinfo.LastError;
  response += F ("\",\r\n\"TICLabels\":[{\r\n");

  index = tinfo.getIndexNextItem (
            0); // search for 1st item
  // Loop thru the TIC items
  while (tinfo.GetItem (index, &labelitem) == true)
  {
    // First item do not add , separator
    if (first_item)
    {
      first_item = false;
    }
    else
    {
      response += F (",\r\n");
    }

    response += F ("\"");
    response += labelitem.name;
    response += F ("\":{\"flags\": ");
    response += labelitem.flags;
    response += F (", \"value\": ");
    formatNumberJSON (response, labelitem.value);
    if (labelitem.horodate.DST[0] !=
        '\0') // must be not 0 or there is nothing in horodate
    {
      response += F (", \"horodate\": \"20");
      response += labelitem.horodate.Year[0];
      response += labelitem.horodate.Year[1];
      response += F ("/");
      response += labelitem.horodate.Month[0];
      response += labelitem.horodate.Month[1];
      response += F ("/");
      response += labelitem.horodate.Day[0];
      response += labelitem.horodate.Day[1];
      response += F (" ");
      response += labelitem.horodate.Hour[0];
      response += labelitem.horodate.Hour[1];
      response += F (":");
      response += labelitem.horodate.Min[0];
      response += labelitem.horodate.Min[1];
      response += F (":");
      response += labelitem.horodate.Sec[0];
      response += labelitem.horodate.Sec[1];
      response += F ("\"");
    }
    response += F ("}");

    // go to next item
    index = tinfo.getIndexNextItem (index);
  }
  // Json end
  response += F ("}]");
  response += FPSTR (FP_JSON_END) ;
  server.send ( 200, "text/json", response );
}

/* ======================================================================
Function: getSysJSONData
Purpose : Return JSON string containing system data
Input   : Response String
Output  : -
Comments: -
====================================================================== */
void getSysJSONData (String& response)
{
  response = "";
  char buffer[32];
  FSInfo info;

  UpdateSysinfo();
  SPIFFS.info (info);
  // Json start
  response += F ("[\r\n");

  response += "{\"na\":\"Uptime\",\"va\":\"";
  response += sysinfo.sys_uptime;
  response += "\"},\r\n";

  response +=
    "{\"na\":\"TICWifi Version\",\"va\":\""
    WIFINFO_VERSION "\"},\r\n";

  response += "{\"na\":\"Compiled date\",\"va\":\""
              __DATE__ " " __TIME__ "\"},\r\n";

  response += "{\"na\":\"SDK Version\",\"va\":\"";
  response += system_get_sdk_version() ;
  response += "\"},\r\n";

  response += "{\"na\":\"Chip ID\",\"va\":\"";
  sprintf_P (buffer, "0x%0X",
             system_get_chip_id() );
  response += buffer ;
  response += "\"},\r\n";

  response += "{\"na\":\"Boot Version\",\"va\":\"";
  sprintf_P (buffer, "0x%0X",
             system_get_boot_version() );
  response += buffer ;
  response += "\"},\r\n";

  response +=
    "{\"na\":\"Flash Real Size\",\"va\":\"";
  response += formatSize (
                ESP.getFlashChipRealSize() ) ;
  response += "\"},\r\n";

  response += "{\"na\":\"Firmware Size\",\"va\":\"";
  response += formatSize (ESP.getSketchSize() ) ;
  response += "\"},\r\n";

  response += "{\"na\":\"Free Size\",\"va\":\"";
  response += formatSize (ESP.getFreeSketchSpace() )
              ;
  response += "\"},\r\n";

  response += "{\"na\":\"TIC Date\",\"va\":\"";
  response += sysinfo.TICDate;
  response += "\"},\r\n";

  response +=
    "{\"na\":\"Jeedom last err\",\"va\":\"";
  sprintf_P (buffer, "%u", sysinfo.jeedom_POSTret);
  response += buffer ;
  response += "\"},\r\n";

  response += "{\"na\":\"SPIFFS Total\",\"va\":\"";
  response += formatSize (info.totalBytes) ;
  response += "\"},\r\n";

  response += "{\"na\":\"SPIFFS Used\",\"va\":\"";
  response += formatSize (info.usedBytes) ;
  response += "\"},\r\n";

  response +=
    "{\"na\":\"SPIFFS Occupation\",\"va\":\"";
  sprintf_P (buffer, "%d%%",
             100 * info.usedBytes / info.totalBytes);
  response += buffer ;
  response += "\"},\r\n";

  // Free mem should be last one
  response += "{\"na\":\"Free Ram\",\"va\":\"";
  response += formatSize (
                system_get_free_heap_size() ) ;
  response +=
    "\"}\r\n"; // Last don't have comma at end

  // Json end
  response += F ("]\r\n");
}

/* ======================================================================
Function: sysJSONTable
Purpose : dump all sysinfo values in JSON table format for browser
Input   : -
Output  : -
Comments: -
====================================================================== */
void sysJSONTable()
{
  String response = "";

  getSysJSONData (response);

  server.send ( 200, "text/json", response );
}



/* ======================================================================
Function: getConfigJSONData
Purpose : Return JSON string containing configuration data
Input   : Response String
Output  : -
Comments: -
====================================================================== */
void getConfJSONData (String& r)
{
  // Json start
  r = FPSTR (FP_JSON_START);
  r += "\"";
  r += CFG_FORM_SSID;
  r += FPSTR (FP_QCQ);
  r += config.Wifi_ssid;
  r += FPSTR (FP_QCNL);
  //r+=CFG_FORM_PSK;       r+=FPSTR(FP_QCQ); r+=config.psk;            r+= FPSTR(FP_QCNL); //don't send it for security reason
  r += CFG_FORM_HOST;
  r += FPSTR (FP_QCQ);
  r += config.Wifi_host;
  r += FPSTR (FP_QCNL);
  r += CFG_FORM_AP_PSK;
  r += FPSTR (FP_QCQ);
  r += config.Wifi_ap_psk;
  r += FPSTR (FP_QCNL);
  r += CFG_FORM_EMON_HOST;
  r += FPSTR (FP_QCQ);
  r += config.emoncms_host;
  r += FPSTR (FP_QCNL);
  r += CFG_FORM_EMON_PORT;
  r += FPSTR (FP_QCQ);
  r += config.emoncms_port;
  r += FPSTR (FP_QCNL);
  r += CFG_FORM_EMON_URL;
  r += FPSTR (FP_QCQ);
  r += config.emoncms_url;
  r += FPSTR (FP_QCNL);
  r += CFG_FORM_EMON_KEY;
  r += FPSTR (FP_QCQ);
  r += config.emoncms_apikey;
  r += FPSTR (FP_QCNL);
  r += CFG_FORM_EMON_NODE;
  r += FPSTR (FP_QCQ);
  r += config.emoncms_node;
  r += FPSTR (FP_QCNL);
  r += CFG_FORM_EMON_FREQ;
  r += FPSTR (FP_QCQ);
  r += config.emoncms_freq;
  r += FPSTR (FP_QCNL);
  r += CFG_FORM_JDOM_HOST;
  r += FPSTR (FP_QCQ);
  r += config.jeedom_host;
  r += FPSTR (FP_QCNL);
  r += CFG_FORM_JDOM_PORT;
  r += FPSTR (FP_QCQ);
  r += config.jeedom_port;
  r += FPSTR (FP_QCNL);
  r += CFG_FORM_JDOM_URL;
  r += FPSTR (FP_QCQ);
  r += config.jeedom_url;
  r += FPSTR (FP_QCNL);
  r += CFG_FORM_JDOM_KEY;
  r += FPSTR (FP_QCQ);
  r += config.jeedom_apikey;
  r += FPSTR (FP_QCNL);
  r += CFG_FORM_JDOM_ADCO;
  r += FPSTR (FP_QCQ);
  r += config.jeedom_adco;
  r += FPSTR (FP_QCNL);
  r += CFG_FORM_JDOM_FREQ;
  r += FPSTR (FP_QCQ);
  r += config.jeedom_freq;
  r += F ("\"");
  // Json end
  r += FPSTR (FP_JSON_END);
}

/* ======================================================================
Function: confJSONTable
Purpose : dump all config values in JSON table format for browser
Input   : -
Output  : -
Comments: -
====================================================================== */
void confJSONTable()
{
  String response = "";
  getConfJSONData (response);
  server.send ( 200, "text/json", response );
}

/* ======================================================================
Function: getSpiffsJSONData
Purpose : Return JSON string containing list of SPIFFS files
Input   : Response String
Output  : -
Comments: -
====================================================================== */
void getSpiffsJSONData (String& response)
{
  char buffer[32];
  bool first_item = true;
  FSInfo info;

  SPIFFS.info (info);
  // Json start
  response = FPSTR (FP_JSON_START);

  // Files Array
  response += F ("\"files\":[\r\n");

  // Loop trough all files
  Dir dir = SPIFFS.openDir ("/");
  while (dir.next() )
  {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    if (first_item)
    {
      first_item = false;
    }
    else
    {
      response += ",";
    }

    response += F ("{\"na\":\"");
    response += fileName.c_str();
    response += F ("\",\"va\":\"");
    response += fileSize;
    response += F ("\"}\r\n");
  }
  response += F ("],\r\n");


  // SPIFFS File system array
  response += F ("\"spiffs\":[\r\n{");

  // Get SPIFFS File system informations
  response += F ("\"Total\":");
  response += info.totalBytes ;
  response += F (", \"Used\":");
  response += info.usedBytes ;
  response += F (", \"ram\":");
  response += system_get_free_heap_size() ;
  response += F ("}\r\n]");

  // Json end
  response += FPSTR (FP_JSON_END);
}

/* ======================================================================
Function: spiffsJSONTable
Purpose : dump all spiffs system in JSON table format for browser
Input   : -
Output  : -
Comments: -
====================================================================== */
void spiffsJSONTable()
{
  String response = "";
  getSpiffsJSONData (response);
  server.send ( 200, "text/json", response );
}

/* ======================================================================
Function: wifiScanJSON
Purpose : scan Wifi Access Point and return JSON code
Input   : -
Output  : -
Comments: -
====================================================================== */
void wifiScanJSON (void)
{
  String response = "";
  bool first = true;

  int n = WiFi.scanNetworks();

  // Json start
  response += F ("[\r\n");

  for (uint8_t i = 0; i < n; ++i)
  {
    int8_t rssi = WiFi.RSSI (i);

    uint8_t percent;

    // dBm to Quality
    if (rssi <= -100)
    {
      percent = 0;
    }
    else if (rssi >= -50)
    {
      percent = 100;
    }
    else
    {
      percent = 2 * (rssi + 100);
    }

    if (first)
    {
      first = false;
    }
    else
    {
      response += F (",");
    }

    response += F ("{\"ssid\":\"");
    response += WiFi.SSID (i);
    response += F ("\",\"rssi\":");
    response += rssi;
    response += FPSTR (FP_JSON_END);
  }

  // Json end
  response += FPSTR ("]\r\n");

  server.send ( 200, "text/json", response );
}


/* ======================================================================
Function: handleFactoryReset
Purpose : reset the module to factory setting
Input   : -
Output  : -
Comments: -
====================================================================== */
void handleFactoryReset (void)
{
  ResetConfig();
  ESP.eraseConfig();
  server.send ( 200, "text/plain",
                FPSTR (FP_RESTART) );
  delay (1000);
  ESP.restart();
  while (true)
  {
    delay (1);
  }
}

/* ======================================================================
Function: handleReset
Purpose : reset the module
Input   : -
Output  : -
Comments: -
====================================================================== */
void handleReset (void)
{
  server.send ( 200, "text/plain",
                FPSTR (FP_RESTART) );
  delay (1000);
  ESP.restart();
  while (true)
  {
    delay (1);
  }
}

/* ======================================================================
Function: handleNotFound
Purpose : default WEB routing when URI is not found
Input   : -
Output  : -
Comments: -
====================================================================== */
void handleNotFound (void)
{
  boolean found = false;
  uint8_t index;
  uint8_t i;
  const char* uri;
  String response;
  sValueList labelitem;

  // try to return SPIFFS file
  found = handleFileRead (server.uri() );
  if (!found)
  {
    // Try to find a Teleinfo Label
    // convert uri to char * for compare
    uri = server.uri().c_str();
    // Got at least one and consistent URI ? increment uri so the '/' is not included in comparison
    if (uri && *uri == '/' && *++uri)
    {
      // We check for an known label
      if (tinfo.GetItem (tinfo.SearchLabel (
                           const_cast<char*> (uri) ), &labelitem) == true)
      {
        // Got it, send json
        response += FPSTR (FP_JSON_START);
        response += F ("\"");
        response += labelitem.name;
        response += F ("\":{\"flags\": ");
        response += labelitem.flags;
        response += F (", \"value\": ");
        formatNumberJSON (response, labelitem.value);
        if (labelitem.horodate.DST[0] !=
            '\0') // must be not 0 or there is nothing in horodate
        {
          response += F (", \"horodate\": \"20");
          response += labelitem.horodate.Year[0];
          response += labelitem.horodate.Year[1];
          response += F ("/");
          response += labelitem.horodate.Month[0];
          response += labelitem.horodate.Month[1];
          response += F ("/");
          response += labelitem.horodate.Day[0];
          response += labelitem.horodate.Day[1];
          response += F (" ");
          response += labelitem.horodate.Hour[0];
          response += labelitem.horodate.Hour[1];
          response += F (":");
          response += labelitem.horodate.Min[0];
          response += labelitem.horodate.Min[1];
          response += F (":");
          response += labelitem.horodate.Sec[0];
          response += labelitem.horodate.Sec[1];
          response += F ("\"");
        }
        response += F ("}");
        response += FPSTR (FP_JSON_END);
        server.send (200, "text/json", response);
        found = true;
      }
    }
  }
  // Nothing found
  if (!found)
  {
    // send error response in plain text
    response = F ("File Not Found\n\n");
    response += F ("URI: ");
    response += server.uri ();
    response += F ("\nMethod: ");
    response += (server.method() == HTTP_GET) ?
                "GET" : "POST";
    response += F ("\nArguments: ");
    response += server.args();
    response += FPSTR (FP_NL);
    for (i = 0; i < server.args(); i++)
    {
      response += " " + server.argName (i) + ": " +
                  server.arg (i) + FPSTR (FP_NL);
    }
    server.send (404, "text/plain", response);
  }

}