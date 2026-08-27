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
  // Next bytes: Analog axis en triggers
  uint8_t x;   // Left stick X
  uint8_t y;   // Left stick Y
  uint8_t rx;  // Right stick X
  uint8_t ry;  // Right stick Y
  uint8_t rz;  // Left analog trigger
  uint8_t z;   // Right analog trigger

  uint16_t all_buttons; 

} raphnet_wusbmote_report_t;

#endif // RAPHNET_WUSBMOTE_H