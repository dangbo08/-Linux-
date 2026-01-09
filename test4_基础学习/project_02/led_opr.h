#ifndef _LED_OPR_H
#define _LED_OPR_H

struct led_operations {
    int (*init)(int which);
    int (*ctl)(int which,char status);
};
#endif
