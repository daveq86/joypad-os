// raphnet_wusbmote.c
#include "raphnet_wusbmote.h"
#include "core/buttons.h"
#include "core/router/router.h"
#include "core/input_event.h"

#include <string.h>

// Macro om bits uit de 16-bit knoppen-masker te halen
#define GET_BIT(mask, button_num) (((mask) >> ((button_num) - 1)) & 0x01)

bool is_raphnet_wusbmote(uint16_t vid, uint16_t pid) {
  return (vid == 0x289B && pid == 0x0080); 
}

bool diff_report_wusbmote(raphnet_wusbmote_report_t const* rpt1, raphnet_wusbmote_report_t const* rpt2) {
  return memcmp(rpt1, rpt2, sizeof(raphnet_wusbmote_report_t)) != 0;
}

void process_raphnet_wusbmote(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
  uint32_t buttons = 0;
  static raphnet_wusbmote_report_t prev_report[MAX_DEVICES] = { 0 };

  raphnet_wusbmote_report_t input_report;
  
  uint16_t copy_len = len < sizeof(raphnet_wusbmote_report_t) ? len : sizeof(raphnet_wusbmote_report_t);
  memset(&input_report, 0, sizeof(raphnet_wusbmote_report_t));
  memcpy(&input_report, report, copy_len);

  if (diff_report_wusbmote(&prev_report[dev_addr-1], &input_report)) {
    uint16_t mask = input_report.all_buttons;
    // 1-on-1 map of Dpad and facebuttons
    buttons = (((GET_BIT(mask, 13)) ? JP_BUTTON_DU : 0) | // button13 =  Up (Dpad)
               ((GET_BIT(mask, 14)) ? JP_BUTTON_DD : 0) | // button14 =  Down (Dpad)
               ((GET_BIT(mask, 15)) ? JP_BUTTON_DL : 0) | // button15 =  Left (Dpad)
               ((GET_BIT(mask, 16)) ? JP_BUTTON_DR : 0) | // button16 =  Right (Dpad)  
               ((GET_BIT(mask, 4))  ? JP_BUTTON_S2 : 0) | // button4  =  + Plus (Start)
               ((GET_BIT(mask, 3))  ? JP_BUTTON_S1 : 0) | // button3  =  - Minus (Select)
               ((GET_BIT(mask, 5))  ? JP_BUTTON_B2 : 0) | // button5  =  a (East)  | GC Style = B4 | Sw Style = B2
               ((GET_BIT(mask, 2))  ? JP_BUTTON_B1 : 0) | // button2  =  b (South) | GC Style = B2 | Sw Style = B1
               ((GET_BIT(mask, 1))  ? JP_BUTTON_B3 : 0) | // button1  =  y (West)  | GC Style = B1 | Sw Style = B3
               ((GET_BIT(mask, 9))  ? JP_BUTTON_B4 : 0) | // button6  =  x (North) | GC Style = B3 | Sw Style = B4
    // Shoulder buttons and Digital triggers
               ((GET_BIT(mask, 9))  ? JP_BUTTON_L1 : 0) | // button9  =  ZL
               ((GET_BIT(mask, 10)) ? JP_BUTTON_R1 : 0) | // button10 =  ZR
               ((GET_BIT(mask, 7))  ? JP_BUTTON_L2 : 0) | // button7  =  L (Digital Click)
               ((GET_BIT(mask, 8))  ? JP_BUTTON_R2 : 0) | // button8  =  R (Digital Click)
               ((GET_BIT(mask, 11)) ? JP_BUTTON_A4 : 0)); // button11 =  Home (A1/A4 mappingtest)

    // Analoge axis and Analog triggers
    uint8_t axis_x  = input_report.x;   // Left Stick X
    uint8_t axis_y  = input_report.y;   // Left Stick Y  
    uint8_t axis_z  = input_report.rx;  // Right Stick X
    uint8_t axis_rz = input_report.ry;  // Right Stick Y
    uint8_t trigger_l = 0;              // Analog Trigger L
    uint8_t trigger_r = 0;              // Analog Trigger R

    if (input_report.rz > 145) {
      uint8_t raw_l = input_report.rz > 249 ? 249 : input_report.rz;
      trigger_l = ((raw_l - 145) * 255) / (249 - 145);
    }

    if (input_report.z > 145) {
      uint8_t raw_r = input_report.z > 239 ? 239 : input_report.z;
      trigger_r = ((raw_r - 145) * 255) / (239 - 145);
    }

    // keep analog within range [1-255]
    ensureAllNonZero(&axis_x, &axis_y, &axis_z, &axis_rz);

    input_event_t event = {
      .dev_addr = dev_addr,
      .instance = instance,
      .type = INPUT_TYPE_GAMEPAD,
      .transport = INPUT_TRANSPORT_USB,
      .layout = LAYOUT_NINTENDO_4FACE,
      .buttons = buttons,
      .button_count = 15,
      .analog = {axis_x, axis_y, axis_z, axis_rz, trigger_l, trigger_r},
      .keys = 0,
    };
    router_submit_input(&event);

    prev_report[dev_addr-1] = input_report;
  }
}

DeviceInterface raphnet_wusbmote_interface = {
  .name = "Raphnet WUSBMote",
  .is_device = is_raphnet_wusbmote,
  .process = process_raphnet_wusbmote,
  .task = NULL,
  .init = NULL
};