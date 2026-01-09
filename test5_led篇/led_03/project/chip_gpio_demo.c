#include "led_opr.h"
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>



static struct led_resource *led_res;
static struct led_operations *led_opr;


static int board_demo_led_init(int which)
{
    if (!led_res)
        return -ENODEV;
    printk("init gpio: group %d, pin %d\n", GROUP(led_res->pin), PIN(led_res->pin));
	switch(GROUP(led_res->pin))
	{
		case 0:
		{
			printk("init pin of group 0 ...\n");
			break;
		}
		case 1:
		{
			printk("init pin of group 1 ...\n");
			break;
		}
		case 2:
		{
			printk("init pin of group 2 ...\n");
			break;
		}
		case 3:
		{
			printk("init pin of group 3 ...\n");
			break;
		}
	}
    return 0;
}

static int board_demo_led_ctl(int which, char status)
{
    //printk("%s %s line %d, led %d, %s\n", __FILE__, __FUNCTION__, __LINE__, which, status ? "on" : "off");
	printk("set led %s: group %d, pin %d\n", status ? "on" : "off", GROUP(led_res->pin), PIN(led_res->pin));

	switch(GROUP(led_res->pin))
	{
		case 0:
		{
			printk("set pin of group 0 ...\n");
			break;
		}
		case 1:
		{
			printk("set pin of group 1 ...\n");
			break;
		}
		case 2:
		{
			printk("set pin of group 2 ...\n");
			break;
		}
		case 3:
		{
			printk("set pin of group 3 ...\n");
			break;
		}
	}

    return 0;
}


static struct led_operations board_demo_led_opr = {
    .init = board_demo_led_init,
    .ctl  = board_demo_led_ctl,
};
struct led_resource *get_led_resource(void)
{
	return led_res;
}
EXPORT_SYMBOL(get_led_resource);

void set_led_resource(struct led_resource *res)
{
	led_res = res;
}
EXPORT_SYMBOL(set_led_resource);

void set_led_opr(struct led_operations *opr)
{
	led_opr = opr;
}
EXPORT_SYMBOL(set_led_opr);

struct led_operations *get_led_opr(void)
{
	return led_opr;
}
EXPORT_SYMBOL(get_led_opr);


/*
void set_led_resource(struct led_resource* led_resource)
{
    led_res = led_resource;
}
void set_led_opr(struct led_operations*led_opr){
    led_opr_board = led_opr;
}*/

static int __init gpio_init(void)
{
	set_led_opr(&board_demo_led_opr);
	return 0;
}

static void __exit gpio_exit(void)
{

}
module_init(gpio_init);
module_exit(gpio_exit);
MODULE_LICENSE("GPL");
