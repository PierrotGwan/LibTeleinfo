// **********************************************************************************
// ESP8266 Teleinfo WEB Client functions
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

#include "webclient.h"

HTTPClient http;

/* ======================================================================
Function: httpPost_
Purpose : Do a http POST
Input   : hostname
          port
          url
Output  : true if received 200 OK
Comments: -
====================================================================== */
boolean httpPost_ (char* host, uint16_t port, char* url, char* payload)
{
  int httpretCode;
  bool ret = false;

  // configure traged server and url
  http.begin (host, port, url);
  http.addHeader ("Content-Type", "application/json"); //Specify content-type header
  // send POST request
  httpretCode = http.POST (payload);
  if (httpretCode)
  {
    // POST Request has been send and Server response has been handled
    // OK Response
    sysinfo.jeedom_POSTret = httpretCode;
    if (httpretCode > 200 && httpretCode < 300)
    {
      //retstring = http.getString ();
      ret = true;
    }
  }
  http.end ();
  return ret;
}

/* ======================================================================
Function: httpGet
Purpose : Do a http GET
Input   : hostname
          port
          url
Output  : true if received 200 OK
Comments: -
====================================================================== */
boolean httpGet (char* host, uint16_t port, char* url)
{
  int httpCode;
  bool ret = false;
  String payload;

  // configure traged server and url
  http.begin (host, port, url);

  // start connection and send HTTP header
  httpCode = http.GET ();
  if (httpCode)
  {
    // HTTP header has been send and Server response header has been handled
    // file found at server
    if (httpCode == 200)
    {
      payload = http.getString ();
      ret = true;
    }
  }
  http.end();
  return ret;
}

/* ======================================================================
Function: ValueIsNumber
Purpose : Test if value is number and return true
Input   : pointer to value to test
Output  : true if value is number
Comments: -
====================================================================== */
boolean ValueIsNumber (char* value)
{
  char* p;
  boolean isNumber = false;

  // we have at least something ?
  if (value)
  {
    if (strlen (value) > 0 && strlen (value) <= 16)
    {
      p = value;
      isNumber = true;
      // check if value is number
      while (*p && isNumber)
      {
        if ( *p < '0' || *p > '9' )
        {
          isNumber = false;
        }
        p++;
      }
    }
  }
  return isNumber;
}

/* ======================================================================
Function: returnNumberJSON
Purpose : Test if value is number and return value without leading zero
Input   : pointer to String for writing number, pointer to value to test
Output  : true if value is number
Comments: -
====================================================================== */
boolean returnNumberJSON (String& response, char* value)
{
  char* p;
  boolean isNumber = false;

  // we have at least something ?
  if (value)
  {
    if (strlen (value) > 0 && strlen (value) <= 16)
    {
      p = value;
      isNumber = true;
      // check if value is number
      while (*p && isNumber)
      {
        if ( *p < '0' || *p > '9' )
        {
          isNumber = false;
        }
        p++;
      }

      // this will remove leading zero on numbers
      if (isNumber)
      {
        p = value;
        while (*p == '0' && * (p + 1) )
        {
          p++;
        }
        response += p ;
      }
    }
  }
  return isNumber;
}

/* ======================================================================
Function: emoncmsPost
Purpose : Do a http GET to emoncms server
Input   :
Output  : true if post returned 200 OK
Comments: -
====================================================================== */
boolean emoncmsPost (void)
{
  boolean ret = false;
  boolean first_item;
  int code;
  char* p;
  uint8_t index;
  sValueList labelitem;
  String url;
  String jsonnumber;

  // Some basic checking
  if (strlen (config.emoncms_host) > 0)
  {
    index = tinfo.getIndexNextItem (
              0); // search for 1st item
    // Got at least one ?
    if (index > 0)
    {
      if (strlen (config.emoncms_url) > 0)
      {
        url = String (config.emoncms_url);
      }
      else
      {
        url = "/";
      }
      url += "?";
      if (config.emoncms_node > 0)
      {
        url += F ("node=");
        url += String (config.emoncms_node);
        url += "&";
      }

      url += F ("apikey=") ;
      url += String (config.emoncms_apikey);
      url += F ("&fulljson={") ;

      first_item = true;

      // Loop thru the TIC item list
      while (tinfo.GetItem (index, &labelitem) == true)
      {
        // First item do not add , separator
        if (first_item)
        {
          first_item = false;
        }
        else
        {
          url += ",";
        }
        // EMONCMS ne sais traiter que des valeurs numériques
        // pour les label avec valeurs texte comme les tarifs il faudra rajouter des traitements spécifiques
        // n'ayant pas les infos seules les valeurs numériques sont transmises ce qui couvre les valeurs de consommation
        jsonnumber = "";
        if (returnNumberJSON (jsonnumber, labelitem.value) == true)
        {
          url += F ("\"");
          url += String (labelitem.name);
          url += F ("\":");
          url += jsonnumber;
        }
        // go to next node
        index = tinfo.getIndexNextItem (index);
      }

      // Json end
      url += "}";

      ret = httpGet (config.emoncms_host, config.emoncms_port, (char*) url.c_str() ) ;
    }
  }
  return ret;
}

/* ======================================================================
Function: jeedomPost
Purpose : Do a http POST to jeedom server
Input   :
Output  : true if post returned 200 OK
Comments: -
====================================================================== */
boolean jeedomPost (void)
{
  uint8_t index;
  boolean ret = false;
  boolean first_item = true;
  sValueList labelitem;
  String url;
  String payload;

  // Some basic checking
  if (strlen (config.jeedom_host) > 0)
  {
    index = tinfo.getIndexNextItem (
              0); // search for 1st TIC item
    // Got at least one ?
    if (index > 0)
    {
      if (strlen (config.jeedom_url) > 0)
      {
        url = String (config.jeedom_url);
      }
      else
      {
        url = "/";
      }
      if (strlen (config.jeedom_apikey) > 0)
      {
        url += F ("?apikey=");
        url += String (config.jeedom_apikey);
      }
      payload = F ("{\"device\":{\"");
      // Config identifiant forcée ?
      if (strlen (config.jeedom_adco) > 0)
      {
        payload += String (config.jeedom_adco);
        payload += F ("\":{\"device\":\"");
        payload += String (config.jeedom_adco);
        payload += F ("\",");
      }
      else
      {
        if (tinfo.GetItem (tinfo.SearchLabel ("ADSC"),
                           &labelitem) == true)
        {
          payload += returnNumberJSON (payload,
                                       labelitem.value);
          payload += F ("\":{\"device\":\"");
          payload += returnNumberJSON (payload,
                                       labelitem.value);
          payload += F ("\"");
        }
      }
      // Loop thru the TIC item list
      index = tinfo.getIndexNextItem (0);
      while (tinfo.GetItem (index, &labelitem) == true)
      {
        payload += F (",\"");
        payload += String (labelitem.name);
        payload += F ("\":");
        if (ValueIsNumber (labelitem.value) == true)
        {
          returnNumberJSON (payload, labelitem.value);
        }
        else
        {
          payload += F ("\"");
          payload += String (labelitem.value);
          payload += F ("\"");
        }

        // go to next TIC item
        index = tinfo.getIndexNextItem (index);
      }
      payload += F ("}}}");
      ret = httpPost_ (config.jeedom_host,
                       config.jeedom_port, 
					   (char*) url.c_str(),
                       (char*) payload.c_str() ) ;
    }
  }
  return ret;
}

/* ======================================================================
Function: MQTTPublish
Purpose : Do a publish of modified value to MQTT Broker
Input   :
Output  : true if publish OK
Comments: -
====================================================================== */
boolean mqttPublish (void)
{
}