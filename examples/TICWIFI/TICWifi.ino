// **********************************************************************************
// ESP8266 Teleinfo WEB Server main sketch
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
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <FS.h>

// Global project file
#include "TICWifi.h"
#include "LibTeleinfoStd.h"

// Teleinfo
TInfo tinfo;

// sysinfo data
_sysinfo sysinfo;

// LED Blink timers
Ticker LedEsp_ticker;
Ticker LedMcu_ticker;
Ticker Tick_emoncms;
Ticker Tick_jeedom;
Ticker Tick_mqtt;

volatile boolean task_emoncms = false;
volatile boolean task_jeedom  = false;
volatile boolean task_mqtt    = false;


/* ======================================================================
Function: UpdateSysinfo
Purpose : update sysinfo variable sys_uptime
Input   : -
Output  : -
Comments: -
====================================================================== */
void UpdateSysinfo (void)
{
  char buff[9];

  int sec = millis() / 1000; // _uptime_sys;
  int min = sec / 60;
  int hr = min / 60;

  sprintf_P ( buff, PSTR ("%02d:%02d:%02d"), hr, min % 60, sec % 60);
  sysinfo.sys_uptime = buff;
}

/* ======================================================================
Function: Task_emoncms
Purpose : callback of emoncms ticker
Input   :
Output  : -
Comments: Like an Interrupt, need to be short, we set flag for main loop
====================================================================== */
void Task_emoncms()
{
  task_emoncms = true;
}

/* ======================================================================
Function: Task_jeedom
Purpose : callback of jeedom ticker
Input   :
Output  : -
Comments: Like an Interrupt, need to be short, we set flag for main loop
====================================================================== */
void Task_jeedom()
{
  task_jeedom = true;
}

/* ======================================================================
Function: Task_mqtt
Purpose : callback of mqtt ticker
Input   :
Output  : -
Comments: Like an Interrupt, need to be short, we set flag for main loop
====================================================================== */
void Task_mqtt()
{
  task_mqtt = true;
}

/* ======================================================================
Function: LedOFF/ON/Setup
Purpose : function to use ESP-12 LED and NodeMCU LED
Input   : -
Output  : -
Comments: Be aware that NodeMCU LED rely on U1TXD, so no serial on TXD1
====================================================================== */
void LedSetup()
{
  pinMode (ESP_LED_PIN, OUTPUT);    // Initialize GPIO2 pin as an output
  pinMode (LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
}

void LedEspOFF (void)
{
  digitalWrite (ESP_LED_PIN, HIGH);
}

void LedEspON (void)
{
  digitalWrite (ESP_LED_PIN, LOW);
}
void LedMcuOFF (void)
{
  digitalWrite (LED_BUILTIN, HIGH);
}

void LedMcuON (void)
{
  digitalWrite (LED_BUILTIN, LOW);
}

/* ======================================================================
Function: DataCallback
Purpose : callback each time a complete data group is decoded
          (between BGR and EGR flag)
Input   : index of the label, use
Output  : -
Comments: -
====================================================================== */
void DataCallback (uint8_t index)
{
  // Do whatever you want there but use only small code as it block
  // continuation of main decode fonction
}

/* ======================================================================
Function: DataErrorCallback
Purpose : callback each time a data group is decoded with error
          (between BGR and EGR flag)
Input   : number of error
Output  : -
Comments: -
====================================================================== */
void DataErrorCallback (uint8_t error_nb)
{
  // Do whatever you want there but use only small code as it block
  // continuation of main decode fonction
  sysinfo.LastError = error_nb;
  // Light the Red NodeMCU Led
  LedMcuON ();
  // led off after delay
  LedMcu_ticker.once_ms (BLINK_LED_MS, LedMcuOFF);
}

/* ======================================================================
Function: NewFrame
Purpose : call each time a complete Teleinfo frame of data group is decoded
          (between STX and ETX flag)
Input   : -
Output  : -
Comments: -
====================================================================== */
void NewFrame (void)
{
  // Do whatever you want there
  sysinfo.TICDate = tinfo.TICDate; //take date from TIC
}

/* ======================================================================
Function: UpdatedFrame
Purpose : callback when we received a complete teleinfo frame
Input   : -
Output  : -
Comments: it's called only if one data in the frame is different than
          the previous frame.
====================================================================== */
void UpdatedFrame()
{
  // Light the Blue ESP-12 Led
  LedEspON ();
  // led off after delay
  LedEsp_ticker.once_ms (BLINK_LED_MS, LedEspOFF);
}

/* ======================================================================
Function: WifiHandleConn
Purpose : Handle Wifi connection / reconnection
Input   : setup true if we're called 1st Time from setup
Output  : state of the wifi status
Comments: -
====================================================================== */
int WifiHandleConn (boolean setup = false)
{
  int ret;
  uint8_t timeout;
  char ap_ssid[32];

  ret = WiFi.status();
  if (setup)
  {
    // no correct SSID
    if (strlen (config.Wifi_ssid) == 0)
    {
      // Let's see of SDK one is okay
      if (WiFi.SSID() == "")
      {
        //nothing to do
      }
      else
      {
        memset (config.Wifi_psk, 0, CFG_PSK_SIZE + 1);
        // Copy SDK SSID
        strncpy (config.Wifi_ssid, WiFi.SSID().c_str(), CFG_SSID_SIZE + 1);

        // Copy SDK password if any
        if (WiFi.psk() != "")
        {
          strcpy (config.Wifi_psk, WiFi.psk().c_str() );
        }
        // save back new config
        saveConfig();
      }
    }
    else // correct SSID
    {
      // Do we have a PSK ?
      if (strlen (config.Wifi_psk) > 0)
      {
        // protected network
        WiFi.begin (config.Wifi_ssid, config.Wifi_psk);
      }
      else
      {
        // Open network
        WiFi.begin (config.Wifi_ssid);
      }

      timeout = 50; // 50 * 200 ms = 10 sec time out
      // 200 ms loop
      while ( ( (ret = WiFi.status() ) != WL_CONNECTED) && timeout )
      {
        // ESP blue LED
        LedEspON ();
        delay (50);
        LedEspOFF ();
        delay (150);
        timeout--;
      }
    }

    // connected ?
    if (ret == WL_CONNECTED)
    {
      //connected, disable AP, client mode only
      WiFi.mode (WIFI_STA);
    }
    else
    {
      // not connected ? start in AP mode
      // STA+AP Mode without connected to STA, autoconnect will search
      // other frequencies while trying to connect, this is causing issue
      // to AP mode, so disconnect will avoid this
      // Disable auto retry search channel
      WiFi.disconnect();

      // SSID = hostname
      strcpy (ap_ssid, config.Wifi_host);

      // protected network
      if (strlen (config.Wifi_ap_psk) > 0)
      {
        WiFi.softAP (ap_ssid, config.Wifi_ap_psk);
      }
      else
      {
        WiFi.softAP (ap_ssid);
      }
      // Open network
      WiFi.mode (WIFI_AP_STA);
    }

    // NodeMCU LED
    LedMcuON ();
    delay (100);
    LedMcuOFF ();
    delay (200);
  }
  return ret;
}

/* ======================================================================
Function: setup
Purpose : Setup I/O and other one time startup stuff
Input   : -
Output  : -
Comments: -
====================================================================== */
void setup()
{
  uint8_t i;
  char buff[32];

  // Set CPU speed to 160MHz
  system_update_cpu_freq (160);

  // Init the two board Leds, and set Red NodeMCU on
  LedSetup ();
  LedEspOFF ();
  LedMcuOFF ();

  // Check File system init
  if (!SPIFFS.begin () )
  {
    // Serious problem
    for (i = 0; i <= 10; i++) //for 3s blink the two leds
    {
      LedEspON ();
      delay (100);
      LedMcuON ();
      delay (100);
      LedEspOFF ();
      delay (100);
      LedMcuOFF ();
    }
  }

  // Read Configuration from EEP
  if (readConfig () )
  {
    //Ok, blink blue LED
    LedEspON ();
    delay (200);
    LedEspOFF ();
  }
  else
  {
    LedMcuON ();
    delay (200);
    // Reset Configuration
    //ResetConfig ();
    LedMcuOFF ();
  }

  // start Wifi connect or soft AP
  WifiHandleConn (true);

  LedEspON ();

  // Update sysinfo variable sys_uptime
  UpdateSysinfo ();
  sysinfo.LastError = 0;

  server.on ("/", handleRoot);
  server.on ("/config_form.json", handleFormConfig);
  server.on ("/json", sendJSON);
  server.on ("/tinfo.json", tinfoJSONTable);
  server.on ("/system.json", sysJSONTable);
  server.on ("/config.json", confJSONTable);
  server.on ("/spiffs.json", spiffsJSONTable);
  server.on ("/wifiscan.json", wifiScanJSON);
  server.on ("/factory_reset", handleFactoryReset);
  server.on ("/reset", handleReset);

  // handler for the hearbeat
  server.on ("/hb.htm", HTTP_GET, [&]()
  {
    server.sendHeader ("Connection", "close");
    server.sendHeader ("Access-Control-Allow-Origin", "*");
    server.send       (200, "text/html", R"(OK)");
  });

  // All other not known
  server.onNotFound (handleNotFound);

  // serves all SPIFFS Web file with 24hr max-age control
  // to avoid multiple requests to ESP
  server.serveStatic ("/font", SPIFFS, "/font", "max-age=86400");
  server.serveStatic ("/js",   SPIFFS, "/js", "max-age=86400");
  server.serveStatic ("/css",  SPIFFS, "/css", "max-age=86400");
  server.begin (80);

  // Teleinfo is connected to RXD2 (GPIO13) to
  // avoid conflict when flashing, this is why
  // we swap RXD1/RXD1 to RXD2/TXD2
  // Note that TXD2 is not used teleinfo is receive only
  Serial.begin (9600, SERIAL_7E1);
  Serial.swap ();

  // Init teleinfo
  tinfo.init();

  // Attach the callback we need
  tinfo.attachDataError    (DataErrorCallback);
  tinfo.attachData         (DataCallback);
  tinfo.attachNewFrame     (NewFrame);
  tinfo.attachUpdatedFrame (UpdatedFrame);

  // Emoncms Update if needed
  if (config.emoncms_freq)
  {
    Tick_emoncms.attach (config.emoncms_freq, Task_emoncms);
  }

  // Jeedom Update if needed
  if (config.jeedom_freq)
  {
    Tick_jeedom.attach (config.jeedom_freq, Task_jeedom);
  }

  // MQTT Update if needed
  if (config.mqtt_freq)
  {
    Tick_mqtt.attach (config.mqtt_freq, Task_mqtt);
  }

  // Finaly light off the onboard LEDs
  LedEspOFF ();
  LedMcuOFF ();
}

/* ======================================================================
Function: loop
Purpose : infinite loop main code
Input   : -
Output  : -
Comments: -
====================================================================== */
void loop()
{
  char ser_recv;
  _State_e tic_frame_in_progress;

  // Handle teleinfo serial
  if (Serial.available () )
  {
    // Read Serial and process to tinfo
    ser_recv = Serial.read ();
    tic_frame_in_progress = tinfo.process (ser_recv);
  }

  if (tic_frame_in_progress ==
      TINFO_WAIT_STX) //not receiving a frame so handle network stuff
  {
    // Handle Webserver client request
    server.handleClient ();
    // Handle POST to enabled servers
    if (task_emoncms)
    {
      emoncmsPost ();
      task_emoncms = false;
    }
    else if (task_jeedom)
    {
      jeedomPost ();
      task_jeedom = false;
    }
    else if (task_mqtt)
    {
      mqttPublish ();
      task_mqtt = false;
    }
  }
}