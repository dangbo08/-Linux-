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

#include "led_resource.h"

static void led_dev_release(struct device *dev)
{
}
static struct resource resources[] = {
    {
        .start      =       GROUP_PIN(1,3),
        .flags      =       IORESOURCE_IRQ,
        .name       =       "db_led_01",
    },
    {
        .start      =       GROUP_PIN(2,6),
        .flags      =       IORESOURCE_IRQ,
        .name       =       "db_led_02",
    },
};

struct platform_device board_A_dev ={
    .name           =          "db_led",
    /*  计算资源数组中有有多少成员 */
    .num_resources  =           ARRAY_SIZE(resources),
    .resource       =           resources,
    .dev            =           {
        .release    =           led_dev_release,
    },
};
static int __init led_dev_init(void)
{
    int err;
    /* int platform_device_register(struct platform_device *pdev) */
    err = platform_device_register(&board_A_dev);
    return 0;
}

static void __exit led_dev_exit(void)
{
    platform_device_unregister(&board_A_dev);
}
  
module_init(led_dev_init);
module_exit(led_dev_exit);

MODULE_LICENSE("GPL");
