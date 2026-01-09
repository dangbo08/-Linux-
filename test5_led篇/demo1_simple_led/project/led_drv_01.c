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
#include <linux/io.h>

/*  确定主设备号 */
static int major = 0;
/*  定义class */
static struct  class *led_class;

/* registers */
//IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03 地址：I20E_0000h base + 68h offset = 20E_0068h
static volatile unsigned int *IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03;
// GPIO1_GDIR 地址：0x020AC004
static volatile unsigned int *GPIO1_GDIR;
//GPIO1_DR 地址：0x020AC000
static volatile unsigned int *GPIO1_DR;

static ssize_t led_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	
	return 0;
}

static ssize_t led_drv_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	char val;
	int ret;
	/* copy_from_user : get data from app */
	ret = copy_from_user(&val, buf, 1);
	/* to set gpio register: out 1/0 */
	if (val)
	{
		/* set gpio to let led on */
		*GPIO1_DR &= ~(1<<3);
	}
	else
	{
		/* set gpio to let led off */
		*GPIO1_DR |= (1<<3);
	}
	return 1;
}

static int led_drv_open (struct inode *node, struct file *file)
{
    /* 复用为GPIO1 */
	unsigned int val;
    val = *IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03;
	val &= ~(0xf);
	val |= (5);
	*IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03 = val;
    /*设置GPIO1_IO03引脚为输出*/
	*GPIO1_GDIR |= (1 << 3);
	/*设置GPIO1_IO03引脚的默认电平*/
	*GPIO1_DR &= ~(1 << 3);
	return 0;
}

static int led_drv_close (struct inode *node, struct file *file)
{
	return 0;
}

static struct file_operations led_drv = {
	.owner	 = THIS_MODULE,
	.open    = led_drv_open,
	.read    = led_drv_read,
	.write   = led_drv_write,
	.release = led_drv_close,
};

static int __init led_init(void)
{   
    int err;
    /*  注册file_operations */
    major = register_chrdev(0,"led",&led_drv);
    /*  创建led_class */
    led_class = class_create(THIS_MODULE,"led_class");
    err = PTR_ERR(led_class);
    if (IS_ERR(led_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "led");
		return -1;
	}
    /*  创建设备节点 */
    device_create(led_class,NULL,MKDEV(major,0),NULL,"led");
    /* ioremap */
	// SW_MUX_CTL_PAD_GPIO1_IO03 地址：20E_0000h base + 68h offset = 20E_0068h
	IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03 = ioremap(0X020E0000 + 0x68, 4);
	// GPIO1_GDIR 地址：0x020AC004
	GPIO1_GDIR = ioremap(0X0209C004, 4);
	//GPIO1_DR 地址：0x020AC000
	GPIO1_DR  = ioremap(0X0209C000, 4);
	return 0;
}

static void __exit led_exit(void)
{
    /*  先创建的晚卸载 */
    /*  卸载设备节点 */
    device_destroy(led_class, MKDEV(major, 0));
    /*  卸载led_class */
	class_destroy(led_class);
    /*  卸载file_operations */
	unregister_chrdev(major, "led");
}
module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");


