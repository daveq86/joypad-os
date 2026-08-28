// raphnet_wusbmote.h
#ifndef RAPHNET_WUSBMOTE_H
#define RAPHNET_WUSBMOTE_H

#include "../../../hid_device.h"
#include "../../../hid_utils.h"
#include "../../generic/hid_parser.h"
#include "tusb.h"

#define INVALID_REPORT_ID -1
#define DEAD_ZONE 4U

#define MAX_BUTTONS 16
#define HID_DEBUG 1

// Schone en perfect uitgelijnde structuur voor stabiele memcmp-checks
typedef struct
{
  uint8_t all_direction; // Bevat de D-pad richting (bitmask via hat)
  uint32_t all_buttons;  // Bevat alle controller knoppen (bits 0 tot 15)
  uint8_t x;             // Linker stick X
  uint8_t y;             // Linker stick Y
  uint8_t z;             // Rechter stick X
  uint8_t rz;            // Rechter stick Y
  uint8_t rx;            // Linker analoge trigger
  uint8_t ry;            // Rechter analoge trigger
} raphnet_wusbmote_state_t;

extern DeviceInterface raphnet_wusbmote_interface;

#endif
