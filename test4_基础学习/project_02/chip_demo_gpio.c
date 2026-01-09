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
#include <linux/platform_device.h>
#include <linux/ioport.h>

#include "led_drv.h"
#include "led_opr.h"
#include "led_resource.h"

/* ******************************第一步完善gpio的控制********************************* */
static int g_ledpins[100];
static int g_ledcnt = 0;

static int board_led_init(int which)
{
    printk("init gpio: group %d, pin %d\n", GROUP(g_ledpins[which]), PIN(g_ledpins[which] )) ;
    switch(GROUP(g_ledpins[which]))
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
static int board_led_ctl(int which,char status)
{
    printk("set led %s: group %d, pin %d\n", status ? "on" : "off", GROUP(g_ledpins[which]), PIN(g_ledpins[which]));

    switch(GROUP(g_ledpins[which]))
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

static struct led_operations board_demo_opr = {
    .init       =       board_led_init,
    .ctl        =       board_led_ctl,
};
/* ******************************第一步完善gpio的控制********************************* */
/* *********************第二步编写plotform_driver,拿取硬件数据************************ */

static int chip_gpio_probe(struct platform_device*pdev)
{
    //接收硬件资源
    struct resource *res;
    int i = 0;
    while (1)
    {
        res = platform_get_resource(pdev,IORESOURCE_IRQ,i++);
        if(!res)
            break;
        /* 在数组中存入了 GROUP_PIN(x,y)的数值*/
        g_ledpins[g_ledcnt] = res->start;
        led_device_create(g_ledcnt);
        g_ledcnt++;
    }
    
    return 0;
}
static int chip_gpio_remove(struct platform_device*pdev)
{
    struct resource *res;
    int i = 0;

    while (1)
    {
        res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
        if (!res)
            break;
        
        led_device_remove(i);
        i++;
        /* g_ledcnt归位方便下一次使用 */
        g_ledcnt--;
    }
    return 0;
}
static struct platform_driver chip_gpio_driver = {
    .probe          =           chip_gpio_probe,
    .remove         =           chip_gpio_remove,
    .driver         =           {
        .name       =           "db_led",
    },
};
/* *********************第二步编写plotform_driver,拿取硬件数据************************ */
static int __init chip_gpio_init(void)
{
    int err;
    register_led_operations(&board_demo_opr);
    /*
    #define platform_driver_register(drv) \
        __platform_driver_register(drv, THIS_MODULE)
    extern int __platform_driver_register(struct platform_driver *,
                        struct module *);
    需要填入struct platform_driver *
    */
    err = platform_driver_register(&chip_gpio_driver);
    return 0;
}


static void __exit chip_gpio_exit(void)
{
    platform_driver_unregister(&chip_gpio_driver);
}

module_init(chip_gpio_init);
module_exit(chip_gpio_exit);
MODULE_LICENSE("GPL");