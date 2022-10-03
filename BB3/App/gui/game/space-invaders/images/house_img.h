#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif


#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_HOUSE
#define LV_ATTRIBUTE_IMG_HOUSE
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_HOUSE uint8_t house_map[] = {
  0x0f, 0xff, 0xf0, 
  0x1f, 0xff, 0xf8, 
  0x3f, 0xff, 0xfc, 
  0x7f, 0xff, 0xfe, 
  0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 
  0xff, 0x81, 0xff, 
  0xfe, 0x00, 0x7f, 
  0xfc, 0x00, 0x3f, 
  0xf8, 0x00, 0x1f, 
  0xf8, 0x00, 0x1f, 
  0xf0, 0x00, 0x0f, 
};

const lv_img_dsc_t house_img = {
  .header.cf = LV_IMG_CF_ALPHA_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 24,
  .header.h = 20,
  .data_size = 60,
  .data = house_map,
};