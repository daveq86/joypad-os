// wusmote.h
#ifndef WUSMOTE_HEADER_H
#define WUSMOTE_HEADER_H

#include "../../hid_device.h"
#include "../../hid_utils.h"
#include "tusb.h"

#define INVALID_REPORT_ID -1
#define DEAD_ZONE 4U
#define MAX_BUTTONS 16
#define HID_DEBUG 1

#define HID_GAMEPAD  0x00
#define HID_MOUSE    0x01
#define HID_KEYBOARD 0x02

// Wusmote USB identification
#define WUSMOTE_VID 0x289B
#define WUSMOTE_PID 0x0080

typedef union
{
  struct
  {
    bool up : 1;
    bool right : 1;
    bool down : 1;
    bool left : 1;

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

    bool button13 : 1;
    bool button14 : 1;
    bool button15 : 1;
    bool button16 : 1;

    uint8_t x;
    uint8_t y;
    uint8_t z;
    uint8_t rz;
    uint8_t rx;
    uint8_t ry;
  };

  struct
  {
    uint8_t all_direction : 4;
    uint32_t all_buttons : 16;
    uint32_t analog_sticks : 32;
    uint16_t analog_triggers : 16;
  };

  uint64_t value : 64;

} dinput_gamepad_t;

extern DeviceInterface raphnet_wusbmote_interface;

#endif