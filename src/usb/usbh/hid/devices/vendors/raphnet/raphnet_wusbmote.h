// raphnet_wusbmote.h
#ifndef RAPHNET_WUSBMOTE_H
#define RAPHNET_WUSBMOTE_H

#include "../../../hid_device.h"
#include "../../../hid_utils.h"
#include "../../generic/hid_parser.h"
#include "tusb.h"

#define INVALID_REPORT_ID -1
#define MAX_BUTTONS 16
#define HID_DEBUG 1

typedef union
{
  struct
  {
    // Individuele bitfields voor directe naamgeving in de .c file
    bool button1 : 1;
    bool button2 : 1;
    bool button3 : 1;
    bool button4 : 1;
    bool button5 : 1;
    bool button6 : 1;
    bool button7 : 1;
    bool button8 : 1;
    bool button9 : 1;
    bool button10 : 1;
    bool button11 : 1;
    bool button12 : 1;
    bool button13 : 1; // Dpad Up
    bool button14 : 1; // Dpad Down
    bool button15 : 1; // Dpad Left
    bool button16 : 1; // Dpad Right
    
    uint16_t padding;   // Vult de struct netjes aan tot 32-bits
  };

  struct
  {
    uint32_t all_buttons; // Gebruikt voor de snelle wijzigingscontrole
  };
} raphnet_wusbmote_state_t;

extern DeviceInterface raphnet_wusbmote_interface;

#endif
