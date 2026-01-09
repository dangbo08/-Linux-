#ifndef _LED_DRV_H
#define _LED_DRV_H
#include "led_opr.h"
void led_device_create(int minor);
void led_device_remove(int minor);
/* 拿到led的信息 */
void register_led_operations(struct led_operations *opr);
#endif