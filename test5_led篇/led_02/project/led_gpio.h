#ifndef _LED_GPIO_H
#define _LED_GPIO_H

#include <linux/module.h>      // 模块宏，module_init/module_exit
#include <linux/kernel.h>      // printk, 内核常用宏
#include <linux/init.h>        // __init, __exit
#include <linux/io.h>          // ioremap, iounmap
#include <linux/types.h>       // u32, u8, NULL
#include <linux/errno.h>       // ENOMEM 等错误码

/*1.设置GPIO地址映射*/
/*2.复用为GPIO*/
/*3.GPIO的输入/输出*/
/*4.GPIO的默认电平*/
/*5.读取GPIO的值*/
/*6.向GPIO写入值*/
/*7.释放映射的虚拟地址*/
/*8.以上封装为一个结构体*/

struct gpio_operations {  
	int (*map_virtual)(void**WHICH_GPIO,unsigned int GPIO_ADDR);  
	int (*reuse)(void*WHICH_GPIO,unsigned int WHICH_MODE,unsigned int gpio_pin,unsigned int bits_ctl);
	int (*direction_out_int)(void*WHICH_GPIO,unsigned int OUT_INT_PUT,unsigned int gpio_pin);
	int (*default_direc)(void*WHICH_GPIO,unsigned int default_dire,unsigned int gpio_pin);
	int (*read_gpio)(void*WHICH_GPIO,unsigned int gpio_pin);  
	int (*write_gpio)(void*WHICH_GPIO,unsigned int VAL,unsigned int gpio_pin);
	int (*release_map)(void**WHICH_GPIO);
};  
struct gpio_operations *get_gpio_opr(void);

#endif