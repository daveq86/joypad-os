// raphnet_wusbmote.c
#include "raphnet_wusbmote.h"
#include "../../generic/hid_parser.h"
#include "core/buttons.h"
#include "core/router/router.h"
#include "core/input_event.h"
#include <string.h>
#include <stdlib.h> 

#define RAPHNET_WUSBMOTE 3

typedef struct
{
  uint8_t  byteIndex;
  uint16_t bitMask;
  uint16_t max;   
  int16_t  min;   
} dinput_usage_t;

typedef struct
{
  dinput_usage_t rxLoc;
  dinput_usage_t ryLoc;
  dinput_usage_t buttonLoc[MAX_BUTTONS]; 
  uint8_t buttonCnt;
  uint8_t type;
  uint8_t report_id;
} dinput_instance_t;

typedef struct
{
  dinput_instance_t instances[5]; 
} dinput_device_t;

static dinput_device_t hid_devices[MAX_DEVICES] = { 0 };

static inline bool USB_GetHIDReportItemInfoWithReportId(const uint8_t *ReportData, HID_ReportItem_t *const ReportItem)
{
  if (HID_DEBUG) TU_LOG1("ReportID: %d ", ReportItem->ReportID);
  return USB_GetHIDReportItemInfo(ReportItem->ReportID, ReportData, ReportItem);
}

static void parse_descriptor(uint8_t dev_addr, uint8_t instance, HID_ReportInfo_t *info)
{
  if (dev_addr >= MAX_DEVICES || instance >= 5) return;
  if (info == NULL || info->FirstReportItem == NULL) return;

  HID_ReportItem_t *item = info->FirstReportItem;
  uint8_t btns_count = 0;
  uint8_t idOffset = 0;

  hid_devices[dev_addr].instances[instance].report_id = item->ReportID;
  if (item->ReportID)
  {
    idOffset = 8;
  }

  while (item)
  {
    uint8_t bitSize = item->Attributes.BitSize ? item->Attributes.BitSize : 0; 
    if (bitSize == 0 || bitSize > 16) {
      item = item->Next;
      continue; 
    }

    uint8_t bitOffset = (item->BitOffset ? item->BitOffset : 0) + idOffset; 
    uint16_t bitMask = ((0xFFFF >> (16 - bitSize)) << (bitOffset % 8)); 
    uint8_t byteIndex = (int)(bitOffset / 8); 

    uint8_t report[] = { item->ReportID, 0 }; 
    if (USB_GetHIDReportItemInfoWithReportId(report, item))
    {
      hid_devices[dev_addr].instances[instance].type = RAPHNET_WUSBMOTE;

      switch (item->Attributes.Usage.Page)
      {
        case HID_USAGE_PAGE_SIMULATE:
        {
          switch (item->Attributes.Usage.Usage)
          {
          case 0xC5: // Brake
            hid_devices[dev_addr].instances[instance].rxLoc.byteIndex = byteIndex;
            hid_devices[dev_addr].instances[instance].rxLoc.bitMask = bitMask;
            hid_devices[dev_addr].instances[instance].rxLoc.max = item->Attributes.Logical.Maximum;
            hid_devices[dev_addr].instances[instance].rxLoc.min = item->Attributes.Logical.Minimum;
            break;
          case 0xC4: // Accelerator
            hid_devices[dev_addr].instances[instance].ryLoc.byteIndex = byteIndex;
            hid_devices[dev_addr].instances[instance].ryLoc.bitMask = bitMask;
            hid_devices[dev_addr].instances[instance].ryLoc.max = item->Attributes.Logical.Maximum;
            hid_devices[dev_addr].instances[instance].ryLoc.min = item->Attributes.Logical.Minimum;
            break;
          default: break;
          }
          break;
        }


//deel2
bool is_raphnet_wusbmote(uint16_t vid, uint16_t pid)
{
  bool match = (vid == 0x289B && pid == 0x0080);
  if (match) {
    TU_LOG1("[RAPHNET] Adapter gevonden! VID: 0x%04X, PID: 0x%04X\r\n", vid, pid);
  }
  return match;
}

bool parse_raphnet_wusbmote(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  HID_ReportInfo_t *local_info = NULL; 
  
  uint8_t ret = USB_ProcessHIDReport(dev_addr, instance, desc_report, desc_len, &local_info);
  if(ret == HID_PARSE_Successful)
  {
    parse_descriptor(dev_addr, instance, local_info);
  }
  else
  {
    TU_LOG1("[RAPHNET] Fout: USB_ProcessHIDReport mislukt met code: %d\r\n", ret);
  }

  if (local_info != NULL) {
    USB_FreeReportInfo(local_info);
  }

  // Controleer of de driver succesvol geactiveerd wordt
  if (dev_addr < MAX_DEVICES && instance < 5 &&
      hid_devices[dev_addr].instances[instance].buttonCnt > 0 &&
      hid_devices[dev_addr].instances[instance].type == RAPHNET_WUSBMOTE) {
    
    TU_LOG1("[RAPHNET] Profiel SUCCESVOL geladen! Aantal knoppen gevonden: %d\r\n", 
            hid_devices[dev_addr].instances[instance].buttonCnt);
    return true;  
  }

  TU_LOG1("[RAPHNET] Profiel GEWEIGERD. Knoppen in descriptor: %d\r\n", 
          hid_devices[dev_addr].instances[instance].buttonCnt);
  return false;
}

//deel3
void process_raphnet_wusbmote(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (dev_addr >= MAX_DEVICES || instance >= 5) return;

  uint32_t buttons = 0;
  static raphnet_wusbmote_state_t previous[MAX_DEVICES][5]; 
  raphnet_wusbmote_state_t current = {0};

  dinput_instance_t *inst = &hid_devices[dev_addr].instances[instance];

  // Definitieve Report ID waarde-fix (voorkomt crashes en loops)
  if (inst->report_id && report[0] != inst->report_id) {
    return;
  }

  uint8_t const* data = report;
  if (inst->report_id) {
    data++; 
  }

  uint16_t rxValue = read_axis_value(data, &inst->rxLoc);
  uint16_t ryValue = read_axis_value(data, &inst->ryLoc);

  // Vult all_buttons bitmask, wat door de union direct doorvloeit naar current.buttonX
  current.all_buttons = 0;
  for (int i = 0; i < MAX_BUTTONS; i++) {
    if (inst->buttonLoc[i].bitMask &&
        (data[inst->buttonLoc[i].byteIndex] & inst->buttonLoc[i].bitMask)) {
      current.all_buttons |= (0x01 << i);
    }
  }

  // Controleert of er een knoptoestand is veranderd
  if (previous[dev_addr][instance].all_buttons != current.all_buttons)
  {
    previous[dev_addr][instance] = current;

    uint8_t buttonCount = inst->buttonCnt;
    if (buttonCount > MAX_BUTTONS) buttonCount = MAX_BUTTONS;

    // HERSTELD: Jouw vertrouwde, originele button-mapping tabel!
    buttons = 
      ((current.button13) ? JP_BUTTON_DU : 0) | // button13 =  Up (Dpad)
      ((current.button14) ? JP_BUTTON_DD : 0) | // button14 =  Down (Dpad)
      ((current.button15) ? JP_BUTTON_DL : 0) | // button15 =  Left (Dpad)
      ((current.button16) ? JP_BUTTON_DR : 0) | // button16 =  Right (Dpad)   
      ((current.button4)  ? JP_BUTTON_S2 : 0) | // button4  =  + Plus (Start)
      ((current.button3)  ? JP_BUTTON_S1 : 0) | // button3  =  - Minus (Select)
      ((current.button5)  ? JP_BUTTON_B2 : 0) | // button5  =  a (East) 
      ((current.button2)  ? JP_BUTTON_B1 : 0) | // button2  =  b (South)
      ((current.button1)  ? JP_BUTTON_B3 : 0) | // button1  =  y (West)  
      ((current.button6)  ? JP_BUTTON_B4 : 0) | // button6  =  x (North) 

      // Schouderknoppen & Triggers
      ((current.button9)  ? JP_BUTTON_L1 : 0) | // button9  =  ZL
      ((current.button10) ? JP_BUTTON_R1 : 0) | // button10 =  ZR
      ((current.button7)  ? JP_BUTTON_L2 : 0) | // button7  =  L (Digital Click)
      ((current.button8)  ? JP_BUTTON_R2 : 0) | // button8  =  R (Digital click)
      ((current.button11) ? JP_BUTTON_A4 : 0);  // button11 =  Home 

    // Analoge simulatie-triggers
    uint8_t trigger_l = inst->rxLoc.bitMask ? (uint8_t)((rxValue * 255) / inst->rxLoc.max) : 0;
    uint8_t trigger_r = inst->ryLoc.bitMask ? (uint8_t)((ryValue * 255) / inst->ryLoc.max) : 0;

    uint8_t axis_x = 128;
    uint8_t axis_y = 128;
    uint8_t axis_z = 128;
    uint8_t axis_rz = 128;

    input_event_t event = {
      .dev_addr = dev_addr,
      .instance = instance,
      .type = INPUT_TYPE_GAMEPAD,
      .transport = INPUT_TRANSPORT_USB,
      .buttons = buttons,
      .button_count = buttonCount,
      .analog = {axis_x, axis_y, axis_z, axis_rz, trigger_l, trigger_r},
      .keys = 0,
    };
    router_submit_input(&event);
  }
}

void unmount_raphnet_wusbmote(uint8_t dev_addr, uint8_t instance)
{
  if (dev_addr < MAX_DEVICES && instance < 5) {
    memset(&hid_devices[dev_addr].instances[instance], 0, sizeof(dinput_instance_t));
  }
}

DeviceInterface raphnet_wusbmote_interface = {
  .name = "Raphnet WUSBMote",
  .is_device = is_raphnet_wusbmote,
  .check_descriptor = parse_raphnet_wusbmote,
  .process = process_raphnet_wusbmote,
  .unmount = unmount_raphnet_wusbmote,
  .init = NULL,
};






//
//    buttons = // 1. Jouw 11 knoppen op pc-posities (Xbox-stijl):
//                ((current.button13) ? JP_BUTTON_DU : 0) | // button13 =  Up (Dpad)
//                ((current.button14) ? JP_BUTTON_DD : 0) | // button14 =  Down (Dpad)
//                ((current.button15) ? JP_BUTTON_DL : 0) | // button15 =  Left (Dpad)
//                ((current.button16) ? JP_BUTTON_DR : 0) | // button16 =  Right (Dpad)   
//                ((current.button4)  ? JP_BUTTON_S2 : 0) | // button4  =  + Plus (Start)
//                ((current.button3)  ? JP_BUTTON_S1 : 0) | // button3  =  - Minus (Select)
//                ((current.button5)  ? JP_BUTTON_B2 : 0) | // button5  =  a (East)  | GC Style = B4 | Sw Style = B2 
//                ((current.button2)  ? JP_BUTTON_B1 : 0) | // button2  =  b (South) | GC Style = B2 | Sw Style = B1
//                ((current.button1)  ? JP_BUTTON_B3 : 0) | // button1  =  y (West)  | GC Style = B1 | Sw Style = B3
//                ((current.button6)  ? JP_BUTTON_B4 : 0) | // button6  =  x (North) | GC Style = B3 | Sw Style = B4
//
//                // Schouderknoppen & Triggers
//                ((current.button9)  ? JP_BUTTON_L1 : 0) | // button9  =  ZL
//                ((current.button10) ? JP_BUTTON_R1 : 0) | // button10 =  ZR
//                ((current.button7)  ? JP_BUTTON_L2 : 0) | // button7  =  L (Digital Click)
//                ((current.button8)  ? JP_BUTTON_R2 : 0) | // button8  =  R (Digital click)
//                ((current.button11) ? JP_BUTTON_A4 : 0);  // button11 =  Home //mapping test A1 / A4
//
//    // 3. Analog Axis (including Analog Triggers)
//    uint8_t axis_x  = current.x;                                 // Left Stick X
//    uint8_t axis_y  = current.y;                                 // Left Stick Y
//    uint8_t axis_z  = inst->zLoc.bitMask ? current.rx : 128;     // Right Stick X
//    uint8_t axis_rz = inst->zLoc.bitMask ? current.ry : 128;     // Right Stick Y
//    uint8_t trigger_l = inst->zLoc.bitMask ? current.rz : 0;     // Analog Trigger L
//    uint8_t trigger_r = inst->zLoc.bitMask ? current.z : 0;      // Analog Trigger R

//    trigger_l = (trigger_l > 145)
//    ? ((trigger_l - 141) * 255) / (249 - 141)
//    : 0;
//
//    trigger_r = (trigger_r > 145)
//    ? ((trigger_r - 141) * 255) / (239 - 141)
//    : 0;

