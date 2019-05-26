// **********************************************************************************
// Reading/Decoding/Storing of French Teleinfo Linky in Standard mode include file
// **********************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CC-BY-SA license:
// http://creativecommons.org/licenses/by-sa/4.0/
//
// For any explanation about teleinfo or use, see my blog
// http://hallard.me/category/tinfo
//
// Code based on following datasheet for Linky
// https://www.enedis.fr/sites/default/files/Enedis-NOI-CPT_54E.pdf
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
// Label TIC principaux :
// DATE : date et heure courante
// EAST : totalisateur consommation en Wh
// EASF01 : totalisateur consommation Heures Creuses
// EASF02 : totalisateur consommation Heures Pleines
// LTARF : nom du tarif en cours (HEURE CREUSE, HEURE PLEINE)
// ADSC : n° du compteur
// URMS1 : Tension efficace instantanée en V
// IRMS1 : Courant efficace instantanée en A
// PREF : Puissance maximale en KVA
// SINSTS1 (ou SINSTS) : Puissance instantanée en VA
// STGE : Registre de status
// **********************************************************************************

#ifndef LibTeleinfoStd_h
#define LibTeleinfoStd_h

#ifdef __arm__
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define boolean bool 
#endif

#ifdef ARDUINO
#include <Arduino.h>
#endif

#ifdef ESP8266
  // For 4 bytes Aligment boundaries
  #define ESP8266_allocAlign(size)  ((size + 3) & ~((size_t) 3))
#endif

#define TINFO_MAXTOKEN      72 //no more than 71 labels in a full TIC frame, just add a 1 more as a precaution

#define TINFO_LABEL_MAXLEN   9  // Max len of label (Doc ENEDIS) + 1 for '\0' terminating string
#define TINFO_HORO_MAXLEN   14  // Max len of Horodate  (Doc ENEDIS) + 1 for '\0' terminating string
#define TINFO_VALUE_MAXLEN  99  // Max len of group value (Label PJOUR+1) + 1 for '\0' terminating string

// state of label
#define TINFO_FLAGS_NOTHING  0x00 //struct index is empty
#define TINFO_FLAGS_NONE     0x01 //struct index has already been added/updated before
#define TINFO_FLAGS_UPDATED  0x02 //struct index just updated
#define TINFO_FLAGS_ADDED    0x04 //struct index just added
#define TINFO_FLAGS_ALERT    0x08 // this struct index must generate an alert

// Local buffer for one line of teleinfo maximum size
#define TINFO_BUFSIZE 128 //TINFO_LABEL_MAXLEN + SEP + TINFO_HORO_MAXLEN + SEP + TINFO_VALUE_MAXLEN + SEP + CHECKSUM rounded to a upper value
// Teleinfo control char of frame
#define TINFO_STX 0x02 // Start of transmission
#define TINFO_ETX 0x03 // End of Transmission
#define TINFO_SGR 0x0A // start of group  
#define TINFO_EGR 0x0D // End of group    
#define TINFO_SEP 0x09 // Separator in Stardard mode

#pragma pack(push)  // push current alignment to stack
#pragma pack(1)     // set alignment to 1 byte boundary

// Library state machine
enum _State_e {
  TINFO_WAIT_NONE,
  TINFO_WAIT_STX, // We're waiting for STX
  TINFO_WAIT_SGR, // We had STX and witing for StartOfGroup
  TINFO_WAIT_EGR, // We had EGR and witing for EndOfGroup
  TINFO_WAIT_ETX  // We had STX, We're waiting for ETX
};

// Array containing all Teleinfo received
typedef struct _ValueList sValueList;
typedef union  _Horodate  uHorodate;

union _Horodate
{
    char rawvalue [TINFO_HORO_MAXLEN];  // raw horodate
    struct {
      char DST   [1]; // 'H' winter time, 'E' summer time, 'h' or 'e' alert that linky is not sync
      char Year  [2];
      char Month [2];
      char Day   [2];
      char Hour  [2];
      char Min   [2];
      char Sec   [2];
      char dummy [1];// for the '\0' ending rawvalue string
  };
};

struct _ValueList 
{
  uint8_t   flags;                         // state flag of value
  char      name     [TINFO_LABEL_MAXLEN]; // Label of value
  char      value    [TINFO_VALUE_MAXLEN]; // value 
  uHorodate horodate;                      // horodate of value
  char      dummy    [5];                  //padding to 128 bytes struct
};

#pragma pack(pop) //return to previous alignement

class TInfo
{
  public:
    TInfo();
    void        init ();  
    _State_e process (char c);
    void        attachData (void (*fn_data)(uint8_t index));  
    void        attachDataError (void (*fn_error)(uint8_t error_nb));  
    void        attachNewFrame (void (*fn_new_frame)(void));
    void        attachUpdatedFrame (void (*fn_updated_frame)(void));  
    uint8_t     SearchLabel (char * name);
    uint8_t     getIndexNextItem (uint8_t index);
    boolean     ValueGet (uint8_t index, char * value);
    boolean     HorodateGet (uint8_t index, uHorodate * horodate);
    boolean     FlagsGet (uint8_t index, uint8_t * flags);
    boolean     GetItem (uint8_t index, sValueList * item);
    uint8_t     labelCount ();
    void        listDelete ();

    String      TICDate; // Date received from Teleinfo

  private:
    void     clearBuffer ();
    uint8_t  AddItem (sValueList * item);
    uint8_t  SetItem (uint8_t index, sValueList * item);
    boolean  ValueSet (uint8_t index, char * value);
    boolean  HorodateSet (uint8_t index, uHorodate * horodate);
    boolean  FlagsSet (uint8_t index, uint8_t flags);
    uint8_t  CheckGroup (void);

    void     (*_fn_data)(uint8_t index);
    void     (*_fn_error)(uint8_t error_nb);
    void     (*_fn_new_frame)(void);
    void     (*_fn_updated_frame)(void);
  
    _State_e _state_group;              // Teleinfo machine state for groups
    _State_e _state_frame;              // Teleinfo machine state for frames
    boolean  _frame_updated;            // Data on the frame has been updated
    uint8_t  _recv_idx;                 // index in receive buffer
    char     _recv_buff[TINFO_BUFSIZE]; // frame receive buffer
};

#endif
