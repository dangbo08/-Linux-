#ifndef _LED_DRV_H
#define _LED_DRV_H

#include "led_opr.h"

void led_class_device_create(int minor);
void led_class_device_remove(int minor);
void register_led_operations(struct led_operations *opr);
#endif /* _LEDDRV_H */

