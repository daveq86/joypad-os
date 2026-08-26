// raphnet_wusbmote.c
#include "raphnet_wusbmote.h"
#include "core/buttons.h"
#include "core/router/router.h"
#include "core/input_event.h"

// Check of het jouw controller is (Vul hier je eigen VID en PID in!)
bool is_raphnet_wusbmote(uint16_t vid, uint16_t pid) {
  return (vid == 0x289B && pid == 0x0080); 
}

// Check of er wijzigingen zijn in de knoppen of assen
bool diff_report_raphnet(raphnet_wusbmote_report_t const* rpt1, raphnet_wusbmote_report_t const* rpt2) {
  return memcmp(rpt1, rpt2, sizeof(raphnet_wusbmote_report_t)) != 0;
}

// Verwerk de USB HID input
void process_raphnet_wusbmote(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
  uint32_t buttons;
  static raphnet_wusbmote_report_t prev_report[MAX_DEVICES] = { 0 };

  raphnet_wusbmote_report_t input_report;
  
  // Veilig overkopiëren naar onze struct
  uint16_t copy_len = len < sizeof(raphnet_wusbmote_report_t) ? len : sizeof(raphnet_wusbmote_report_t);
  memset(&input_report, 0, sizeof(raphnet_wusbmote_report_t));
  memcpy(&input_report, report, copy_len);

  if (diff_report_raphnet(&prev_report[dev_addr-1], &input_report)) {
    
    // 1-on-1 map of Dpad and facebuttons
    buttons = (((input_report.button13) ? JP_BUTTON_DU : 0) | // button13 =  Up (Dpad)
               ((input_report.button14) ? JP_BUTTON_DD : 0) | // button14 =  Down (Dpad)
               ((input_report.button15) ? JP_BUTTON_DL : 0) | // button15 =  Left (Dpad)
               ((input_report.button16) ? JP_BUTTON_DR : 0) | // button16 =  Right (Dpad)  
               ((input_report.button4)  ? JP_BUTTON_S2 : 0) | // button4  =  + Plus (Start)
               ((input_report.button3)  ? JP_BUTTON_S1 : 0) | // button3  =  - Minus (Select)
               ((input_report.button5)  ? JP_BUTTON_B2 : 0) | // button5  =  a (East)  | GC Style = B4 | Sw Style = B2
               ((input_report.button2)  ? JP_BUTTON_B1 : 0) | // button2  =  b (South) | GC Style = B2 | Sw Style = B1
               ((input_report.button1)  ? JP_BUTTON_B3 : 0) | // button1  =  y (West)  | GC Style = B1 | Sw Style = B3
               ((input_report.button6)  ? JP_BUTTON_B4 : 0) | // button6  =  x (North) | GC Style = B3 | Sw Style = B4
    // Shoulde buttons and Digital triggers
               ((input_report.button9)  ? JP_BUTTON_L1 : 0) | // button9  =  ZL
               ((input_report.button10) ? JP_BUTTON_R1 : 0) | // button10 =  ZR
               ((input_report.button7)  ? JP_BUTTON_L2 : 0) | // button7  =  L (Digital Click)
               ((input_report.button8)  ? JP_BUTTON_R2 : 0) | // button8  =  R (Digital Click)
               ((input_report.button11) ? JP_BUTTON_A4 : 0)); // button11 =  Home (A1/A4 mappingtest)

    // Analoge axis and Analog triggers
    uint8_t axis_x  = input_report.x;   // Left Stick X
    uint8_t axis_y  = input_report.y;   // Left Stick Y  
    uint8_t axis_z  = input_report.rx;  // Right Stick X
    uint8_t axis_rz = input_report.ry;  // Right Stick Y
    uint8_t trigger_l = 0;              // Analog Trigger L
    uint8_t trigger_r = 0;              // Analog Trigger R

    if (input_report.rz > 141) {
      uint8_t raw_l = input_report.rz > 249 ? 249 : input_report.rz;
      trigger_l = ((raw_l - 141) * 255) / (249 - 141);
    }

    if (input_report.z > 141) {
      uint8_t raw_r = input_report.z > 239 ? 239 : input_report.z;
      trigger_r = ((raw_r - 141) * 255) / (239 - 141);
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