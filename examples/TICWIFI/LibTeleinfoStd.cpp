// **********************************************************************************
// Reading/Decoding/Storing of French Teleinfo Linky in Standard mode functions
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

#include "LibTeleinfoStd.h" 

sValueList ValuesTab[TINFO_MAXTOKEN];  //Allocate static table of TIC labels (71 labels in Standard mode)

/* ======================================================================
Class   : TInfo
Purpose : Constructor
Input   : -
Output  : -
Comments: -
====================================================================== */
TInfo::TInfo ()
{
  init ();
  // callback
  _fn_data = NULL;   
  _fn_new_frame = NULL;   
  _fn_updated_frame = NULL;
  _fn_error = NULL;
}

/* ======================================================================
Function: init
Purpose : configure values list, receive buffer and State
Input   : -
Output  : -
Comments: - 
====================================================================== */
void TInfo::init ()
{
  // free up linked list (in case on recall init())
  listDelete();

  // clear our receive buffer
  clearBuffer();

  // We're ready to start receiving data
  _state_frame = TINFO_WAIT_STX;
  _state_group = TINFO_WAIT_NONE;
  _frame_updated = false;
  TICDate = "";
}

/* ======================================================================
Function: listDelete
Purpose : Erase the ENTIRE array of TIC labels
Input   : -
Output  : -
====================================================================== */
void TInfo::listDelete ()
{
  uint8_t i;
  
  for (i = 0; i < TINFO_MAXTOKEN; i++) 
  {
    memset (&ValuesTab[i], 0, sizeof(_ValueList));
  }
}

/* ======================================================================
Function: clearBuffer
Purpose : clear and init the buffer
Input   : -
Output  : -
Comments: - 
====================================================================== */
void TInfo::clearBuffer ()
{
  // Clear our buffer, set index to 0
  memset (_recv_buff, 0, TINFO_BUFSIZE);
  _recv_idx = 0;
}

/* ======================================================================
Function: labelCount
Purpose : Count the number of label in the list
Input   : -
Output  : element numbers
====================================================================== */
uint8_t TInfo::labelCount ()
{
  uint8_t count = 0;
  uint8_t i;
  
  for(i = 0; i < TINFO_MAXTOKEN; i++) 
  {
    if (ValuesTab[i].flags > TINFO_FLAGS_NOTHING)
    {
      count++;
    }
  }
  return count;
}

/* ======================================================================
Function: attachDataError 
Purpose : attach a callback when we detected an error on receiving data
Input   : callback function
Output  : - 
Comments: -
====================================================================== */
void TInfo::attachDataError (void (*fn_error)(uint8_t error_nb))  
{
  // indicate the user callback
  _fn_error = fn_error;   
}

/* ======================================================================
Function: attachNewData 
Purpose : attach a callback when we detected a new/changed value 
Input   : callback function
Output  : - 
Comments: -
====================================================================== */
void TInfo::attachData (void (*fn_data)(uint8_t index))
{
  // indicate the user callback
  _fn_data = fn_data;   
}

/* ======================================================================
Function: attachNewFrame 
Purpose : attach a callback when we received a full frame
Input   : callback function
Output  : - 
Comments: -
====================================================================== */
void TInfo::attachNewFrame (void (*fn_new_frame)(void))
{
  // indicate the user callback
  _fn_new_frame = fn_new_frame;   
}

/* ======================================================================
Function: attachChangedFrame 
Purpose : attach a callback when we received a full frame where data
          has changed since the last frame (cool to update data)
Input   : callback function
Output  : - 
Comments: -
====================================================================== */
void TInfo::attachUpdatedFrame (void (*fn_updated_frame)(void))
{
  // indicate the user callback
  _fn_updated_frame = fn_updated_frame;   
}

/* ======================================================================
Function: SearchLabel
Purpose : Search index of element with corresponding Label
Input   : Pointer to the label name
Output  : index + 1 of element, 0 if not found
====================================================================== */
uint8_t TInfo::SearchLabel (char * name)
{
  uint8_t i;

  for (i = 0; i < TINFO_MAXTOKEN; i++) 
  {
    if (ValuesTab[i].flags > TINFO_FLAGS_NOTHING)
    {
      // Check if we match this LABEL
      if (strcmp(ValuesTab[i].name, name) == 0) 
      {
        return (i + 1);
      }
    }
  }
  return 0;
}

/* ======================================================================
Function: getIndexNextItem
Purpose : return position of the next item in list (name not null) after 
          the one indexed by input 
Input   : index to start,
Output  : (index + 1) or 0 if error
Comment : To get first item call this with index = 0
====================================================================== */
uint8_t TInfo::getIndexNextItem (uint8_t index)
{
  uint8_t i;

  for (i = index; i < TINFO_MAXTOKEN; i++) 
  {
    if (ValuesTab[i].flags > TINFO_FLAGS_NOTHING)
    {
      return (i + 1);
    }
  }
  return 0;
}

/* ======================================================================
Function: GetItem
Purpose : copy the indexed item to the given pointer memory
Input   : Index of Item, returned by getIndexNextItem or SearchLabel
Output  : True if ok, False on error 
====================================================================== */
boolean TInfo::GetItem (uint8_t index, sValueList * item)
{
  if (index > 0 && index <= TINFO_MAXTOKEN)
  {
    index--; //real index of item in ValueTab array
    //is item named ? and flags ok ?
    if (ValuesTab[index].name[0] > '\0' && ValuesTab[index].flags > TINFO_FLAGS_NOTHING)
    {
      // copy indexed label struct to item
      memcpy (item, &ValuesTab[index], sizeof(_ValueList));
      return true;
    }
  }
  // index error or item is empty
  return false;
}

/* ======================================================================
Function: SetItem
Purpose : update the indexed item with data of given pointer struct
Input   : Index of Item, returned by getIndexFirstItem or SearchLabel
Output  : 0 nothing modified
          1 if only value modified
          2 if only horodate modified
          3 if value and horodate modified
          0x80 on error : index out of bound or label name different
Comment : set item flags to Updated or None
====================================================================== */
uint8_t TInfo::SetItem (uint8_t index, sValueList * item)
{
  uint8_t mod_label;

  mod_label = 0x80;
  if (index > 0 && index <= TINFO_MAXTOKEN)
  {
    index--; //real index of item in ValueTab array
    if (strcmp (item->name, ValuesTab[index].name) == 0)
    {
      mod_label = 0;
      if (strcmp (item->value, ValuesTab[index].value) != 0)
        mod_label |= 1;
      if (strcmp (item->horodate.rawvalue, ValuesTab[index].horodate.rawvalue) != 0)
        mod_label |= 2;
      if (mod_label > 0) // overwrite label only if something different
      {
        memcpy (&ValuesTab[index], item, sizeof(_ValueList));
        ValuesTab[index].flags = TINFO_FLAGS_UPDATED;
      }
      else
      {
        ValuesTab[index].flags = TINFO_FLAGS_NONE;
      }
    }//different name ! some error
  }
  return mod_label;
}

/* ======================================================================
Function: FlagsGet
Purpose : Get flags of one element
Input   : Index of Item, returned by getIndexFirstItem or SearchLabel
          pointer to a uint8_t where to copy flags
Output  : True if ok, False if error
====================================================================== */
boolean TInfo::FlagsGet (uint8_t index, uint8_t * flags)
{
  if (index > 0 && index <= TINFO_MAXTOKEN)
  {
    index--; //real index of item in ValueTab array
    //is item named ? and flags ok ?
    if (ValuesTab[index].name[0] > '\0' && ValuesTab[index].flags > TINFO_FLAGS_NOTHING)
    {
      // copy flags
      *flags = ValuesTab[index].flags;
      return true;
    }
  }
  // index error or item is empty
  return false;
}
/* ======================================================================
Function: FlagsSet
Purpose : Get flags of one element
Input   : Index of Item, returned by getIndexFirstItem or SearchLabel
          flags to put in item flags
Output  : True if ok, False if error
====================================================================== */
boolean TInfo::FlagsSet (uint8_t index, uint8_t flags)
{
  if (index > 0 && index <= TINFO_MAXTOKEN)
  {
    index--; //real index of item in ValueTab array
    //is item named ? flags could be not ok as we set them...
    if (ValuesTab[index].name[0] > '\0')
    {
      // store new flags
      ValuesTab[index].flags = flags;
      return true;
    }
  }
  // index error or item is empty (unnamed)
  return false;
}

/* ======================================================================
Function: valueGet
Purpose : Get value field of one element
Input   : Index of Item, returned by getIndexFirstItem or SearchLabel
          pointer to a char[TINFO_VALUE_MAXLEN] where to copy data
Output  : True if ok, False if error
====================================================================== */
boolean TInfo::ValueGet (uint8_t index, char * value)
{
  if (index > 0 && index <= TINFO_MAXTOKEN)
  {
    index--; //real index of item in ValueTab array
    if (ValuesTab[index].flags > TINFO_FLAGS_NOTHING)
    {
      // copy to dest buffer
      strncpy (value, ValuesTab[index].value, TINFO_VALUE_MAXLEN);
      return true;
    }
  }
  // index error or item is empty
  return false;
}

/* ======================================================================
Function: valueSet
Purpose : Set value field of one element
Input   : Index of the label name gathered with SearchLabel
          pointer to a char[TINFO_VALUE_MAXLEN] where to copy data
Output  : True if ok, False if error
====================================================================== */
boolean TInfo::ValueSet (uint8_t index, char * value)
{
  if (index > 0 && index <= TINFO_MAXTOKEN)
  {
    index--; //real index of item in ValueTab array
    // copy to dest buffer
    //is item named ? and flags ok ?
    if (ValuesTab[index].name[0] > '\0' && ValuesTab[index].flags > TINFO_FLAGS_NOTHING)
    {
      strncpy (ValuesTab[index].value, value, TINFO_VALUE_MAXLEN);
      return true;
    }
  }
  // index error or item is empty
  return false;
}

/* ======================================================================
Function: HorodateGet
Purpose : get Horodate field of one element
Input   : Index of the label name gathered with SearchLabel
          pointer to an _Horodate struct where to copy data
Output  : True if ok, False if error
====================================================================== */
boolean TInfo::HorodateGet (uint8_t index, uHorodate * horodate)
{
  if (index > 0 && index <= TINFO_MAXTOKEN)
  {
    index--; //real index of item in ValueTab array
    if (ValuesTab[index].flags > TINFO_FLAGS_NOTHING)
    {
      // copy to dest buffer
      strncpy (horodate->rawvalue, ValuesTab[index].horodate.rawvalue, TINFO_HORO_MAXLEN);
      return true;
    }
  }
  // index error or item is empty
  return false;
}

/* ======================================================================
Function: HorodateSet
Purpose : Set Horodate field of one element
Input   : Index of the label name gathered with SearchLabel
          pointer to an _Horodate struct from where to copy data
Output  : True if ok, False if error
====================================================================== */
boolean TInfo::HorodateSet(uint8_t index, uHorodate * horodate)
{
  if (index > 0 && index <= TINFO_MAXTOKEN)
  {
    index--; //real index of item in ValueTab array
    //is item named ? and flags ok ?
    if (ValuesTab[index].name[0] > '\0' && ValuesTab[index].flags > TINFO_FLAGS_NOTHING)
    {
      // copy to field
      strncpy (ValuesTab[index].horodate.rawvalue, horodate->rawvalue, TINFO_HORO_MAXLEN);
      return true;
    }
  }
  // index error or item is empty
  return false;
}

/* ======================================================================
Function: AddItem
Purpose : Add a label in TIC array
Input   : Pointer to item to add
Output  : index of stored new item or 0 if error
Comment : 
====================================================================== */
uint8_t TInfo::AddItem (sValueList * item)
{
  uint8_t i = 0;
  uint8_t lghoro = 0;

  //Search for first free entry in array
  for (i = 0; i < TINFO_MAXTOKEN; i++)
  {
    if (ValuesTab[i].flags == TINFO_FLAGS_NOTHING)
    {
      memcpy (&ValuesTab[i], item, sizeof(_ValueList));
      ValuesTab[i].flags = TINFO_FLAGS_ADDED;
      i ++; //increment as 0 is for error
      break;
    }
  }
  if (i >= TINFO_MAXTOKEN)//no more space to add label !
    i = 0; //return error
  // return index + 1 of the label added/updated or 0 if error
  return (i);
}

/* ======================================================================
Function: CheckGroup
Purpose : check one group of teleinfo received between SGR and EGR flag
Input   : -
Output  : bit field of errors
Comments: 
====================================================================== */
uint8_t TInfo::CheckGroup (void) 
{
  uint8_t index;
  uint8_t flags;
  uint8_t errnb;
  int buff_index;
  int count_SEP;
  char checksum; //calculated checksum from buffer
  char recv_checksum; //received checksum from transmission
  char buff_char;
  int tmp_readvalue_index;
  char tmp_readvalue [TINFO_VALUE_MAXLEN];
  sValueList recv_item;
  
  // Init values
  errnb = 0;
  checksum = 0;
  recv_checksum = 0;
  count_SEP = 0;
  tmp_readvalue_index = 0;
  memset (tmp_readvalue, 0, TINFO_VALUE_MAXLEN);
  memset (&recv_item, 0, sizeof(sValueList));
  flags = TINFO_FLAGS_NONE;
  
  for (buff_index = 0; buff_index < _recv_idx; buff_index++)
  {
    buff_char = _recv_buff[buff_index];
    // Separator ?
    if (buff_char == TINFO_SEP)
    {
      count_SEP ++;
      checksum += TINFO_SEP; // need for checksum calc
      tmp_readvalue [tmp_readvalue_index] = '\0'; //just a precaution
      switch (count_SEP)
      {
        case 1:
		  if (tmp_readvalue_index < TINFO_LABEL_MAXLEN) //with use of 'strncpy' we cannot know if there an error in length of received data so test it
		  {
            strncpy (recv_item.name, tmp_readvalue, TINFO_LABEL_MAXLEN); //strncpy pad end of destination with \0 
		  }
		  else
		  {
			errnb = 1;
		  }
          break;
		  
        case 2:
          if (_recv_buff[buff_index + 2] == TINFO_EGR) // end of group after 2 char so data is value and not horodate
          {
            strncpy (recv_item.value, tmp_readvalue, TINFO_VALUE_MAXLEN);
		    recv_item.value [TINFO_VALUE_MAXLEN-1] = '\0'; //parano�a
          }
          else
          {
		    if (tmp_readvalue_index < TINFO_HORO_MAXLEN) //with use of 'strncpy' we cannot know if there an error in length of received data so test it
		    {
              strncpy (recv_item.horodate.rawvalue, tmp_readvalue, TINFO_HORO_MAXLEN);
			}
			else
			{
			  errnb = 1;
			}
          }
          break;
		  
        case 3:
          //third SEP so data is value
          strncpy (recv_item.value, tmp_readvalue, TINFO_VALUE_MAXLEN);
		  recv_item.value [TINFO_VALUE_MAXLEN-1] = '\0'; //parano�a
          break;

        default:
          //well, more than three SEP, there is an error !
          errnb |= 1;
      }
      //reinit buffer readvalue and index
      tmp_readvalue_index = 0;
	  memset (tmp_readvalue, 0, TINFO_VALUE_MAXLEN);
    }
    else if (buff_char == TINFO_EGR)
    {  
      recv_checksum = _recv_buff[buff_index - 1]; //Checksum is just before end of groupe marker
      checksum -= recv_checksum; // as we have added before in default case
      checksum &= 0x3F; //truncate to 6 bits
      checksum += 0x20; // do it an ASCII car
      break; //exit loop as there will be nothing after !
    }
    else
    {
      if (tmp_readvalue_index < TINFO_VALUE_MAXLEN-1)
      {
        tmp_readvalue [tmp_readvalue_index++] = buff_char;
        checksum += buff_char;
      }
	  else
	  {
		errnb = 1; //some problem as data overload max size of values
	  }
    }
	if (errnb != 0) break; //exit for if any error
  }
  // no error and at least received Name + Value 
  if (errnb == 0 && count_SEP >= 2) //if count_SEP > 3 then errnb = 1
  {
    // calc checksum is ok ?
    if (checksum == recv_checksum) 
    {
      //Label DATE is used only to update member TICDate and not put in array of labels
      if (strcmp (recv_item.name, "DATE") == 0) 
      {
        //format horodate : (H/E)AAMMDDHHMMSS
        TICDate = "20";
        TICDate += recv_item.horodate.Year[0];
        TICDate += recv_item.horodate.Year[1];
        TICDate += "/";
        TICDate += recv_item.horodate.Month[0];
        TICDate += recv_item.horodate.Month[1];
        TICDate += "/";
        TICDate += recv_item.horodate.Day[0];
        TICDate += recv_item.horodate.Day[1];
        TICDate += " ";
        TICDate += recv_item.horodate.Hour[0];
        TICDate += recv_item.horodate.Hour[1];
        TICDate += ":";
        TICDate += recv_item.horodate.Min[0];
        TICDate += recv_item.horodate.Min[1];
        TICDate += ":";
        TICDate += recv_item.horodate.Sec[0];
        TICDate += recv_item.horodate.Sec[1];
      }
      else
      {
        // Add value to array of teleinfo labels
        index = SearchLabel (recv_item.name); //is this Label name already exist ?
        if (index == 0)
        {
          // new label, add it in TIC array
          index = AddItem (&recv_item);
          if (index == 0)
          {
            flags = 0;
            errnb |= 8;
          }
          else
          {
            flags = 3;
          }
        }
        else
        {
          // update data
          flags = SetItem (index, &recv_item);
          if (flags > 0x7F) //error
          {
            errnb |= 4;
            flags = 0;
          }
        }
        //some modification done ?
        if (flags > 0) 
        {
          // this label have been updated/added, so frame at least contains an update
          _frame_updated = true;
          //callback for this label if needed
          if (_fn_data)
            _fn_data(index);
        }
      }
    }
    else
    {
      errnb |= 2;
    }
  }

  return errnb;
}

/* ======================================================================
Function: process
Purpose : teleinfo serial char received processing, should be called
          my main loop, this will take care of managing all the other
Input   : pointer to the serial used 
Output  : teleinfo global state
====================================================================== */
_State_e TInfo::process(char c)
{
  uint8_t error_cg;
  
  // be sure 7 bits only
  c &= 0x7F;
  // What we received ?
  switch (c)  
  {
    // start of transmission
    case  TINFO_STX:
      // Clear buffer, begin to store in it
      clearBuffer();
      // by default frame is not "updated", if data change we'll set this flag
      _frame_updated = false;
      _state_frame = TINFO_WAIT_ETX;
      _state_group = TINFO_WAIT_SGR;
    break;
      
    // Start of group
    case  TINFO_SGR:
	  if (_state_frame == TINFO_WAIT_ETX) //only if we have received an STX
	  {
        _state_group = TINFO_WAIT_EGR; //ok for start receiving a group and waiting for EGR
	  }
	  else
	  {
		_state_group = TINFO_WAIT_NONE; // partial reception of frame, ignore until a new STX
	  }
      clearBuffer(); // in all case clear buffer by precaution      
    break;

    // End of group
    case  TINFO_EGR:
      // Normal working mode ?
	  if (_state_frame == TINFO_WAIT_ETX) //we have received Start of Frame and waiting for End of Frame
	  {
        if (_state_group == TINFO_WAIT_EGR) // AND we have received Start of Group and waiting for End of Group
        {
          if (_recv_idx < TINFO_BUFSIZE - 1)
          {
            _recv_buff[_recv_idx++] = c; //used in CheckGroup
            _recv_buff[_recv_idx] = '\0'; //finishing buffer with 0x00
            // check the group we've just received
            error_cg = CheckGroup ();
            if (error_cg > 0)
            {
              if (_fn_error)
              _fn_error (error_cg);
            }
          }
          _state_group = TINFO_WAIT_SGR; // waiting for another group or ETX
        }
	  }
	  else
	  {
        _state_group = TINFO_WAIT_NONE; //partial receive of frame, state of group is none
	  }
      clearBuffer(); //  Whatever error or not, we've done so clear the buffer
    break;
    
    // End of transmission
    case  TINFO_ETX:
      // Normal working mode ?
      if (_state_frame == TINFO_WAIT_ETX) //normal mode, end of frame
      {
        // Call user callback if any
        if (_frame_updated == true)
        {
          _frame_updated = false;
          if (_fn_updated_frame)
          {
            _fn_updated_frame();
          }
        }
        if (_fn_new_frame)
        {
          _fn_new_frame();
        }
	  }
	  clearBuffer();  
      _state_frame = TINFO_WAIT_STX;
      _state_group = TINFO_WAIT_NONE;
    break;

    // other char ?
    default:
    {
      // Only if between SGR and EGR
      if (_state_group == TINFO_WAIT_EGR) 
      {
        // If buffer is not full, Store data 
        if ( _recv_idx < TINFO_BUFSIZE)
        {
          _recv_buff[_recv_idx++] = c;
        }
        else //problem of more data than normal, reseting states and buffer
        {
		  clearBuffer();
          _state_frame = TINFO_WAIT_STX;
          _state_group = TINFO_WAIT_NONE;
        }
      }
    }
    break;
  }
  return _state_frame;
}
