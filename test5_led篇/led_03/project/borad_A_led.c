#include "led_resource.h"
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
static struct led_resource board_A_led = {
    .pin = GROUP_PIN(1,3),
};

static int __init board_A_init(void)
{
	set_led_resource(&board_A_led);
	return 0;
}

static void __exit board_A_exit(void)
{

}
module_init(board_A_init);
module_exit(board_A_exit);
MODULE_LICENSE("GPL");