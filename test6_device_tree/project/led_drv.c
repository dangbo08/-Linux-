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

#include "led_opr.h"

static int major;
static struct class *led_class;
struct led_operations *p_led_opr;

void led_class_device_create(int minor)
{
    device_create(led_class, NULL, MKDEV(major, minor), NULL, "db_led%d", minor); /* /dev/100ask_led0,1,... */
}
void led_class_device_remove(int minor)
{
    device_destroy(led_class, MKDEV(major, minor));
}
void register_led_operations(struct led_operations *opr)
{
	p_led_opr = opr;
}
EXPORT_SYMBOL(led_class_device_create);
EXPORT_SYMBOL(led_class_device_remove);
EXPORT_SYMBOL(register_led_operations);

/* 3. 实现对应的open/read/write等函数，填入file_operations结构体                   */
static ssize_t led_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

/* write(fd, &val, 1); */
static ssize_t led_drv_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	int err;
	char status;
	struct inode *inode = file_inode(file);
	int minor = iminor(inode);
	
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	err = copy_from_user(&status, buf, 1);

	/* 根据次设备号和status控制LED */
	p_led_opr->ctl(p_led_opr->data,minor, status);
	
	return 1;
}

static int led_drv_open (struct inode *node, struct file *file)
{
	int minor = iminor(node);
	
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	/* 根据次设备号初始化LED */
	p_led_opr->init(p_led_opr->data,minor);
	
	return 0;
}

static int led_drv_close (struct inode *node, struct file *file)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

static struct file_operations led_opr = {
    .owner	 = THIS_MODULE,
	.open    = led_drv_open,
	.read    = led_drv_read,
	.write   = led_drv_write,
	.release = led_drv_close,
};

static int __init led_init(void)
{
    int err;
    major = register_chrdev(0,"db_led",&led_opr);
    led_class = class_create(THIS_MODULE,"db_led_class");
    err = PTR_ERR(led_class);
	if (IS_ERR(led_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "db_led");
		return -1;
	}
	return 0;
}

static void __exit led_exit(void)
{
	class_destroy(led_class);
	unregister_chrdev(major, "db_led");
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");


