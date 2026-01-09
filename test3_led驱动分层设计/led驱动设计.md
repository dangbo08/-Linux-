# led驱动设计

# 1.上一讲回顾

```c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <asm/io.h>

/* registers */
//SW_MUX_CTL_PAD_GPIO1_IO03 地址：I20E_0000h base + 68h offset = 20E_0068h
static volatile unsigned int *SW_MUX_CTL_PAD_GPIO1_IO03;

// GPIO5_GDIR 地址：0x020AC004
static volatile unsigned int *GPIO1_GDIR;

//GPIO5_DR 地址：0x020AC000
static volatile unsigned int *GPIO1_DR;

static unsigned int major;
static struct class *led_class;

static ssize_t led_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	unsigned int str  = ((*GPIO1_DR >> 3) & 0x1);
	copy_to_user(buf,&str,sizeof(str));
	return sizeof(str);
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
	return sizeof(val);
}

static int led_drv_open (struct inode *node, struct file *file)
{
	//初始化
	*SW_MUX_CTL_PAD_GPIO1_IO03 &= ~0xf;
	*SW_MUX_CTL_PAD_GPIO1_IO03 |= 0x5;

	*GPIO1_GDIR |= (1<<3);
	//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

static int led_drv_close (struct inode *node, struct file *file)
{
	//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

struct file_operations led_opr = {
	.owner 		=		THIS_MODULE,
	.open 		=  		led_drv_open,
	.read 		=  		led_drv_read,
	.write 		=   	led_drv_write,
	.release 	=  	 	led_drv_close,
};

static int __init led_init(void)
{
	//1.向内核注册一个字符设备号 + 操作函数表  内核层（VFS）
	major = register_chrdev(0,"myled",&led_opr);

	/* ioremap */
	// SW_MUX_CTL_PAD_GPIO1_IO03 地址：20E_0000h base + 68h offset = 20E_0068h
	SW_MUX_CTL_PAD_GPIO1_IO03 = ioremap(0X020E0000 + 0x68, 4);
	
	// GPIO1_GDIR 地址：0x020AC004
	GPIO1_GDIR = ioremap(0X0209C004, 4);
	
	//GPIO1_DR 地址：0x020AC000
	GPIO1_DR  = ioremap(0X0209C000, 4);

	//2.在 /sys/class 中创建一个类目录，用于用户空间管理   sysfs 层（/sys/class）
	led_class = class_create(THIS_MODULE,"led_class");
	//3.在 /dev 下创建设备文件，供用户访问   设备节点层（/dev）
	device_create(led_class, NULL, MKDEV(major, 0), NULL, "led");
	return 0;
}


static void __exit led_exit(void)
{
	//在入口函数中创建了，就得在出口函数中销毁
	device_destroy(led_class, MKDEV(major, 0));
	class_destroy(led_class);
	unregister_chrdev(major, "led");
}
module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
```

​	阅读源码将上一讲简单的驱动程序回顾一下，上一讲中我们已经能够实现led的亮灭实验，但是这样非常不方便，如果我们有多个led灯，怎么办？需要写多个一样的代码吗？显然这是不合理的，所以我们需要将这些相同的部分抽象出来，在提供不同的部分；这里和STM32非常相似，我们需要自己封装一个GPIO的库，和底板的库，GPIO的库就是一样的，使用不同的底板时就提供不同的底板程序，可能相同的芯片但是开发板厂家封装的pin脚不一样，也可能不同的芯片寄存器相关名称也不一致。（注意：这个也是比较原始的写法，与现在直接进行修改设备树还是相差很大）

# 2.驱动设计框架

![](D:\学习笔记\驱动学习_重新开始\test3_led驱动分层设计\截图\设计图.png)

​	上面驱动的operations结构体，同理我们也可以定义一个led的opr结构体：

```c
struct imx_led_operations 
{
	int num;							//led的编号  多个led
	int (*init)(int which);				//初始化  which--哪一个led
	int (*ctl)(int which , int status);	//控制led which--哪一个led，status--led的状态
	int (*raed)(int which);				//读取led的状态 	which--哪一个led
};
```

