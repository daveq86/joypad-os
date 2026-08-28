// raphnet_wusbmote.c
#include "raphnet_wusbmote.h"
#include "../../generic/hid_parser.h"
#include "core/buttons.h"
#include "core/router/router.h"
#include "core/input_event.h"
#include <string.h>
#include <stdlib.h> // Toegevoegd voor abs()

#define RAPHNET_WUSBMOTE 3

typedef struct
{
  uint8_t  byteIndex;
  uint16_t bitMask;
  uint16_t max;   // logical maximum (16-bit is plenty for gamepad axes)
  int16_t  min;   // logical minimum — <0 marks a signed/centered axis
} dinput_usage_t;

// Generic HID instance state
typedef struct
{
  dinput_usage_t xLoc;
  dinput_usage_t yLoc;
  dinput_usage_t zLoc;
  dinput_usage_t rzLoc;
  dinput_usage_t rxLoc;
  dinput_usage_t ryLoc;
  dinput_usage_t hatLoc;
  dinput_usage_t buttonLoc[MAX_BUTTONS]; // assuming a maximum of 16 buttons
  uint8_t buttonCnt;
  uint8_t type;
  bool xbox_axes;  
  uint8_t report_id;
} dinput_instance_t;

// Cached device report properties on mount
typedef struct
{
  dinput_instance_t instances[5]; // CRUCIALE FIX: Nu een echte array van 5 instanties!
} dinput_device_t;

static dinput_device_t hid_devices[MAX_DEVICES] = { 0 };

//(hat format, 8 is released, 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW)
static const uint8_t HAT_SWITCH_TO_DIRECTION_BUTTONS[] = {0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001, 0b0000};

// Gets HID descriptor report item for specific ReportID
static inline bool USB_GetHIDReportItemInfoWithReportId(const uint8_t *ReportData, HID_ReportItem_t *const ReportItem)
{
  if (HID_DEBUG) TU_LOG1("ReportID: %d ", ReportItem->ReportID);
  return USB_GetHIDReportItemInfo(ReportItem->ReportID, ReportData, ReportItem);
}

// Parses HID descriptor into byteIndex/buttonMasks
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

    uint8_t report[2] = { item->ReportID, 0 }; 
    if (USB_GetHIDReportItemInfoWithReportId(report, item))
    {
      hid_devices[dev_addr].instances[instance].type = RAPHNET_WUSBMOTE;

      switch (item->Attributes.Usage.Page)
      {
        case HID_USAGE_PAGE_DESKTOP:
        {
          switch (item->Attributes.Usage.Usage)
          {
          case HID_USAGE_DESKTOP_X:
            hid_devices[dev_addr].instances[instance].xLoc.byteIndex = byteIndex;
            hid_devices[dev_addr].instances[instance].xLoc.bitMask = bitMask;
            hid_devices[dev_addr].instances[instance].xLoc.max = item->Attributes.Logical.Maximum;
            hid_devices[dev_addr].instances[instance].xLoc.min = item->Attributes.Logical.Minimum;
            break;
          case HID_USAGE_DESKTOP_Y:
            hid_devices[dev_addr].instances[instance].yLoc.byteIndex = byteIndex;
            hid_devices[dev_addr].instances[instance].yLoc.bitMask = bitMask;
            hid_devices[dev_addr].instances[instance].yLoc.max = item->Attributes.Logical.Maximum;
            hid_devices[dev_addr].instances[instance].yLoc.min = item->Attributes.Logical.Minimum;
            break;
          case HID_USAGE_DESKTOP_Z:
            hid_devices[dev_addr].instances[instance].zLoc.byteIndex = byteIndex;
            hid_devices[dev_addr].instances[instance].zLoc.bitMask = bitMask;
            hid_devices[dev_addr].instances[instance].zLoc.max = item->Attributes.Logical.Maximum;
            hid_devices[dev_addr].instances[instance].zLoc.min = item->Attributes.Logical.Minimum;
            break;
          case HID_USAGE_DESKTOP_RZ:
            hid_devices[dev_addr].instances[instance].rzLoc.byteIndex = byteIndex;
            hid_devices[dev_addr].instances[instance].rzLoc.bitMask = bitMask;
            hid_devices[dev_addr].instances[instance].rzLoc.max = item->Attributes.Logical.Maximum;
            hid_devices[dev_addr].instances[instance].rzLoc.min = item->Attributes.Logical.Minimum;
            break;
          case HID_USAGE_DESKTOP_RX:
            hid_devices[dev_addr].instances[instance].rxLoc.byteIndex = byteIndex;
            hid_devices[dev_addr].instances[instance].rxLoc.bitMask = bitMask;
            hid_devices[dev_addr].instances[instance].rxLoc.max = item->Attributes.Logical.Maximum;
            hid_devices[dev_addr].instances[instance].rxLoc.min = item->Attributes.Logical.Minimum;
            break;
          case HID_USAGE_DESKTOP_RY:
            hid_devices[dev_addr].instances[instance].ryLoc.byteIndex = byteIndex;
            hid_devices[dev_addr].instances[instance].ryLoc.bitMask = bitMask;
            hid_devices[dev_addr].instances[instance].ryLoc.max = item->Attributes.Logical.Maximum;
            hid_devices[dev_addr].instances[instance].ryLoc.min = item->Attributes.Logical.Minimum;
            break;
          case HID_USAGE_DESKTOP_HAT_SWITCH:
            hid_devices[dev_addr].instances[instance].hatLoc.byteIndex = byteIndex;
            hid_devices[dev_addr].instances[instance].hatLoc.bitMask = bitMask;
            break;
          default: break;
          }
          break;
        }

//deel2
        case HID_USAGE_PAGE_BUTTON:
        {
          uint8_t usage = item->Attributes.Usage.Usage;
          if (usage >= 1 && usage <= MAX_BUTTONS) {
            hid_devices[dev_addr].instances[instance].buttonLoc[usage - 1].byteIndex = byteIndex;
            hid_devices[dev_addr].instances[instance].buttonLoc[usage - 1].bitMask = bitMask;
          }
          btns_count++;
          break;
        }
        default: break;
      }
    }
    item = item->Next;
  }

  hid_devices[dev_addr].instances[instance].buttonCnt = btns_count;

  // Xbox Axis Remapping beveiligd voor Raphnet
  dinput_instance_t *inst = &hid_devices[dev_addr].instances[instance];
  if (inst->type != RAPHNET_WUSBMOTE && inst->rxLoc.max && inst->ryLoc.max && !inst->rzLoc.max) {
    inst->xbox_axes = true;
    dinput_usage_t old_z = inst->zLoc;
    inst->zLoc = inst->rxLoc;     
    inst->rzLoc = inst->ryLoc;    
    inst->rxLoc = old_z;          
    inst->ryLoc = (dinput_usage_t){0};  
  }
}

bool is_raphnet_wusbmote(uint16_t vid, uint16_t pid)
{
  return (vid == 0x289B && pid == 0x0080);
}

// Gecorrigeerde parser met lokale info-pointer
bool parse_raphnet_wusbmote(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  HID_ReportInfo_t *local_info = NULL; 
  
  uint8_t ret = USB_ProcessHIDReport(dev_addr, instance, desc_report, desc_len, &local_info);
  if(ret == HID_PARSE_Successful)
  {
    parse_descriptor(dev_addr, instance, local_info);
  }

  if (local_info != NULL) {
    USB_FreeReportInfo(local_info);
  }

  if (dev_addr < MAX_DEVICES && instance < 5 &&
      hid_devices[dev_addr].instances[instance].buttonCnt > 0 &&
      hid_devices[dev_addr].instances[instance].type == RAPHNET_WUSBMOTE) {
    return true;  
  }

  return false;
}

uint8_t scale_analog_raphnet_wusbmote(uint16_t value, uint32_t max_value)
{
  if (max_value == 0) return 128;
  int mid_point = max_value / 2;
  if (value <= mid_point) {
    return 1 + (value * 127) / mid_point;
  } else {
    return 128 + ((value - mid_point) * 127) / (max_value - mid_point);
  }
}

static uint16_t read_axis_value(const uint8_t *report, const dinput_usage_t *loc)
{
  if (!loc->bitMask) return 0;

  if (loc->bitMask > 0xFF) {
    uint16_t combined = (uint16_t)report[loc->byteIndex] | ((uint16_t)report[loc->byteIndex + 1] << 8);
    uint16_t masked = combined & loc->bitMask;
    return masked >> __builtin_ctz(loc->bitMask);
  } else {
    uint8_t masked = report[loc->byteIndex] & loc->bitMask;
    return masked >> __builtin_ctz(loc->bitMask);
  }
}

static uint8_t scale_axis(const dinput_usage_t *loc, uint16_t raw)
{
  if (loc->min < 0) {
    int32_t sval = (loc->bitMask > 0xFF) ? (int16_t)raw : (int8_t)raw;
    int32_t span = (int32_t)loc->max - (int32_t)loc->min;
    if (span <= 0) return 128;
    int32_t out = 1 + (int32_t)(sval - loc->min) * 254 / span;
    return out < 1 ? 1 : (out > 255 ? 255 : (uint8_t)out);
  }
  return scale_analog_raphnet_wusbmote(raw, loc->max);
}

//deel3
void process_raphnet_wusbmote(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (dev_addr >= MAX_DEVICES || instance >= 5) return;

  uint32_t buttons = 0;
  // FIX: Nu gedefinieerd als een volwaardige 2D-array [MAX_DEVICES][5]
  static raphnet_wusbmote_state_t previous[MAX_DEVICES][5]; 
  raphnet_wusbmote_state_t current = {0};

  dinput_instance_t *inst = &hid_devices[dev_addr].instances[instance];

  if (inst->report_id && report[0] != inst->report_id) {
    return;
  }

  uint8_t const* data = report;
  if (inst->report_id) {
    data++; 
  }

  uint16_t xValue = read_axis_value(data, &inst->xLoc);
  uint16_t yValue = read_axis_value(data, &inst->yLoc);
  uint16_t zValue = read_axis_value(data, &inst->zLoc);
  uint16_t rzValue = read_axis_value(data, &inst->rzLoc);
  uint16_t rxValue = read_axis_value(data, &inst->rxLoc);
  uint16_t ryValue = read_axis_value(data, &inst->ryLoc);

  uint8_t hatValue = 8;
  if (inst->hatLoc.bitMask) {
    uint8_t rawHat = data[inst->hatLoc.byteIndex] & inst->hatLoc.bitMask;
    hatValue = rawHat <= 8 ? rawHat : 8;
    current.all_direction |= HAT_SWITCH_TO_DIRECTION_BUTTONS[hatValue];
  }

  current.all_buttons = 0;
  for (int i = 0; i < MAX_BUTTONS; i++) {
    if (inst->buttonLoc[i].bitMask &&
        (data[inst->buttonLoc[i].byteIndex] & inst->buttonLoc[i].bitMask)) {
      current.all_buttons |= (0x01 << i);
    }
  }

  current.x  = inst->xLoc.bitMask  ? scale_axis(&inst->xLoc, xValue)   : 128;
  current.y  = inst->yLoc.bitMask  ? scale_axis(&inst->yLoc, yValue)   : 128;
  current.z  = inst->zLoc.bitMask  ? scale_axis(&inst->zLoc, zValue)   : 128;
  current.rz = inst->rzLoc.bitMask ? scale_axis(&inst->rzLoc, rzValue) : 128;
  current.rx = inst->rxLoc.bitMask ? scale_axis(&inst->rxLoc, rxValue) : 0;
  current.ry = inst->ryLoc.bitMask ? scale_axis(&inst->ryLoc, ryValue) : 0;

  bool state_changed = (previous[dev_addr][instance].all_buttons != current.all_buttons) ||
                       (previous[dev_addr][instance].all_direction != current.all_direction) ||
                       (abs((int)previous[dev_addr][instance].x - (int)current.x) > DEAD_ZONE) ||
                       (abs((int)previous[dev_addr][instance].y - (int)current.y) > DEAD_ZONE);

  if (state_changed)
  {
    previous[dev_addr][instance] = current;

    uint8_t buttonCount = inst->buttonCnt;
    if (buttonCount > MAX_BUTTONS) buttonCount = MAX_BUTTONS;

    buttons = 
      ((current.all_direction & 0b0001) ? JP_BUTTON_DU : 0) | 
      ((current.all_direction & 0b0010) ? JP_BUTTON_DR : 0) |
      ((current.all_direction & 0b0100) ? JP_BUTTON_DD : 0) |
      ((current.all_direction & 0b1000) ? JP_BUTTON_DL : 0) |
      
      ((current.all_buttons & (1 << 0))  ? JP_BUTTON_B3 : 0) | 
      ((current.all_buttons & (1 << 1))  ? JP_BUTTON_B1 : 0) | 
      ((current.all_buttons & (1 << 2))  ? JP_BUTTON_S1 : 0) | 
      ((current.all_buttons & (1 << 3))  ? JP_BUTTON_S2 : 0) | 
      ((current.all_buttons & (1 << 4))  ? JP_BUTTON_B2 : 0) | 
      ((current.all_buttons & (1 << 5))  ? JP_BUTTON_B4 : 0) | 
      
      ((current.all_buttons & (1 << 6))  ? JP_BUTTON_L2 : 0) | 
      ((current.all_buttons & (1 << 7))  ? JP_BUTTON_R2 : 0) | 
      ((current.all_buttons & (1 << 8))  ? JP_BUTTON_L1 : 0) | 
      ((current.all_buttons & (1 << 9))  ? JP_BUTTON_R1 : 0) | 
      ((current.all_buttons & (1 << 10)) ? JP_BUTTON_A4 : 0);  

    uint8_t axis_x  = current.x;
    uint8_t axis_y  = current.y;
    uint8_t axis_z  = inst->zLoc.bitMask ? current.rx : 128;
    uint8_t axis_rz = inst->zLoc.bitMask ? current.ry : 128;
    uint8_t trigger_l = inst->zLoc.bitMask ? current.rz : 0;
    uint8_t trigger_r = inst->zLoc.bitMask ? current.z : 0;

    trigger_l = (trigger_l > 145) ? ((trigger_l - 141) * 255) / (249 - 141) : 0;
    trigger_r = (trigger_r > 145) ? ((trigger_r - 141) * 255) / (239 - 141) : 0;

    ensureAllNonZero(&axis_x, &axis_y, &axis_z, &axis_rz);

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

