#ifndef _BOARD_IMX_H
#define _BOARD_IMX_H
#include "led_gpio.h"
struct led_operations {
	int num;                                            //次设备号的数值    也就是需要几个led灯
	int (*init)(int which);							    //init--初始化LED，	which--哪一个LED
	int (*ctl)(int wshich , char status);			    //ctl--控制LED，	which--哪一个LED，status--状态--0-灭/1-亮
	int (*read)(int wshich);							//red--读取LED电平，	which--哪一个LED
};

#define GPIO1_BASEADDR      0X0209C000
#define GPIO2_BASEADDR      0X020A0000
#define GPIO3_BASEADDR      0X020A4000
#define GPIO4_BASEADDR      0X020A8000
#define GPIO5_BASEADDR      0X020AC000
#define GPIO_REG_SIZE       0X1000

typedef struct {
    volatile unsigned int DR;        // 0x00
    volatile unsigned int GDIR;      // 0x04
    volatile unsigned int PSR;       // 0x08
    volatile unsigned int ICR1;      // 0x0C
    volatile unsigned int ICR2;      // 0x10
    volatile unsigned int IMR;       // 0x14
    volatile unsigned int ISR;       // 0x18
    volatile unsigned int EDGE_SEL;  // 0x1C
} GPIOx_Func;


struct led_operations *get_board_led_opr(void);
void delete_io(void);
#endif