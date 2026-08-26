// raphnet_wusbmote.h
#ifndef RAPHNET_WUSBMOTE_H
#define RAPHNET_WUSBMOTE_H

#include "../../../hid_device.h"
#include "../../../hid_utils.h"
#include "tusb.h"

extern DeviceInterface raphnet_wusbmote_interface;

// De exacte bit- en byte-opbouw op basis van jouw controllerdata
typedef struct TU_ATTR_PACKED
{
  // Byte 0: button 1 - 8
  struct {
    uint8_t button1 : 1; // y (West)
    uint8_t button2 : 1; // b (South)
    uint8_t button3 : 1; // - Minus (select)
    uint8_t button4 : 1; // + PLus (Start)
    uint8_t button5 : 1; // a (East)
    uint8_t button6 : 1; // x (North)
    uint8_t button7 : 1; // L (Digital click)
    uint8_t button8 : 1; // R (Digital click)
  };

  // Byte 1: Button 9 - 16
  struct {
    uint8_t button9  : 1; // ZL
    uint8_t button10 : 1; // ZR
    uint8_t button11 : 1; // Home
    uint8_t button12 : 1; // Unused
    uint8_t button13 : 1; // Dpad Up
    uint8_t button14 : 1; // Dpad Down
    uint8_t button15 : 1; // Dpad Left
    uint8_t button16 : 1; // Dpad Right
  };

  // Next bytes: Analog axis en triggers
  uint8_t x;   // Left stick X
  uint8_t y;   // Left stick Y
  uint8_t rx;  // Right stick X
  uint8_t ry;  // Right stick Y
  uint8_t rz;  // Left analog trigger
  uint8_t z;   // Right analog trigger

} raphnet_wusbmote_report_t;


#endif // RAPHNET_WUSBMOTE_H