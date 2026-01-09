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
#include <linux/gpio/consumer.h>
#include <linux/pinctrl/consumer.h>
#include "led_drv.h"

/* 这里需要从设备树文件中判断有几个led,并创建对应个数的节点 */
/****************用作led的计数*****************/
struct board_led {
    struct gpio_desc *gpiod;
};
struct chip_gpio_priv {
    struct board_led *leds;
    int led_count;
};

static struct chip_gpio_priv *g_priv;

/****************用作led的计数*****************/


static int board_demo_led_init (void*data,int which) /* 初始化LED, which-哪个LED */       
{   
    struct chip_gpio_priv *g_priv = data;
    if (!g_priv || which >= g_priv->led_count)
        return -EINVAL;

    /* 设置为输出，默认灭 */
    gpiod_direction_output(g_priv->leds[which].gpiod, 0);
    return 0;
}

static int board_demo_led_ctl (void*data,int which, char status) /* 控制LED, which-哪个LED, status:1-亮,0-灭 */
{
    struct chip_gpio_priv *g_priv = data;
    if (!g_priv || which >= g_priv->led_count)
        return -EINVAL;

    /* status: 1-亮, 0-灭 */
    gpiod_set_value(g_priv->leds[which].gpiod,
                    status ? 1 : 0);
    return 0;
}

static struct led_operations board_demo_led_opr = {
    .init = board_demo_led_init,
    .ctl  = board_demo_led_ctl,
};


/* 经典抄写 */
static int chip_demo_gpio_probe(struct platform_device *pdev)
{
    int i;
    /* 1.拿到device_node */
    struct device *dev = &pdev->dev;
    struct chip_gpio_priv *priv;
    //struct device_node *np = dev->of_node;
    
    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;
    /* 2.数一数有几个led */
    /*int gpiod_count(struct device *dev, const char *con_id) */
    priv->led_count = gpiod_count(dev, "led");
    if (priv->led_count < 0)
        return priv->led_count;
    /* 3.申请数组保存每一个LED的gpio_desc */
    priv->leds = devm_kzalloc(dev,
        sizeof(struct board_led) * priv->led_count,
        GFP_KERNEL);
    if (!priv->leds)
        return -ENOMEM;
    /* 4.逐个解析 led-gpios */
    for (i = 0; i < priv->led_count; i++) {
        priv->leds[i].gpiod = devm_gpiod_get_index(dev,
                            "led", i, GPIOD_OUT_LOW);
        if (IS_ERR(priv->leds[i].gpiod))
            return PTR_ERR(priv->leds[i].gpiod);
        led_class_device_create(i);
    }
    /* 5.把 leds / led_count 保存起来 */
    platform_set_drvdata(pdev, priv);
    board_demo_led_opr.data = priv;
    g_priv = priv;
    register_led_operations(&board_demo_led_opr);
    return 0;
}

static int chip_demo_gpio_remove(struct platform_device *pdev)
{
    int i;
    struct chip_gpio_priv *priv = platform_get_drvdata(pdev);
    for (i = 0; i < priv->led_count; i++)
        led_class_device_remove(i);

    //unregister_led_operations();
    return 0;
}

static const struct of_device_id db_leds[] = {
    {.compatible="db,db_led1"},
    {}
};
MODULE_DEVICE_TABLE(of, db_leds);
static struct platform_driver chip_demo_gpio = {
    .probe      =       chip_demo_gpio_probe,
    .remove     =       chip_demo_gpio_remove,
    .driver     =       {
        .name   =       "db_leds",
        .of_match_table =   db_leds,
    }
};


static int __init gpio_init(void)
{
    int err;
    err = platform_driver_register(&chip_demo_gpio);
    
    return 0;
}

static void __exit gpio_exit(void)
{
    platform_driver_unregister(&chip_demo_gpio);
}


module_init(gpio_init);
module_exit(gpio_exit);
MODULE_LICENSE("GPL");