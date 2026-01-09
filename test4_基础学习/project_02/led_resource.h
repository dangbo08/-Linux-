#ifndef _LED_RESOURCE_H
#define _LED_RESOURCE_H
/* 规定高16位为group，低16位为pin脚 */
#define GROUP(x)                (x>>16)
#define PIN(x)                  (x&0XFFFF)
#define GROUP_PIN(x,y)          ((x<<16) | (y))
#endif