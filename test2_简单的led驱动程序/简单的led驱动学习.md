# 简单的led驱动学习

# 1.回顾字符设备驱动程序的框架

​	回顾上一节写的字符设备的驱动程序框架

![](D:\学习笔记\驱动学习_重新开始\test2_简单的led驱动程序\截图\分层.png)

​	其实就是分为app和驱动层，他不涉及硬件，但是led就比这个hello驱动多出了一个硬件层。

![](D:\学习笔记\驱动学习_重新开始\test2_简单的led驱动程序\截图\硬件分层.png)

​	这里就意味着我们需要将硬件部分结合起来了，回顾我们写的hello驱动代码。

```c
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
static unsigned int major = 0;
static char kernel_buf[1024];
static struct class *hello_class;


#define MIN(a, b) (a < b ? a : b)


static ssize_t hello_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	int err;
	//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	err = copy_to_user(buf, kernel_buf, MIN(1024, size));
	return MIN(1024, size);
}

static ssize_t hello_drv_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	int err;
	//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	err = copy_from_user(kernel_buf, buf, MIN(1024, size));
	return MIN(1024, size);
}

static int hello_drv_open (struct inode *node, struct file *file)
{
	//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

static int hello_drv_close (struct inode *node, struct file *file)
{
	//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}
struct file_operations hello_opr = {
	.owner 		=		THIS_MODULE,
	.open 		=  		hello_drv_open,
	.read 		=  		hello_drv_read,
	.write 		=   	hello_drv_write,
	.release 	=  	 	hello_drv_close,
};
static int __init hello_init(void)
{
	int err;
	//确定主设备号
	//这是向内核注册一个字符设备号 + 操作函数表  内核层（VFS）
	major = register_chrdev(0,"hello",&hello_opr);
	//在 /sys/class 中创建一个类目录，用于用户空间管理   sysfs 层（/sys/class）
	hello_class = class_create(THIS_MODULE, "hello_class");
	err = PTR_ERR(hello_class);
	if (IS_ERR(hello_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "hello");
		return -1;
	}
	//自动在 /dev 下创建设备文件，供用户访问   设备节点层（/dev）
	device_create(hello_class, NULL, MKDEV(major, 0), NULL, "hello"); /* /dev/hello */
	//以上三步它们分别服务于内核识别、系统管理、用户访问这三个层面。
	return 0;
}


static void __exit hello_exit(void)
{
	//在入口函数中创建了，就得在出口函数中销毁
	device_destroy(hello_class, MKDEV(major, 0));
	class_destroy(hello_class);
	unregister_chrdev(major, "hello");
}
module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");

```

# 2.led驱动设计--简单版

​	从前面的字符设备回顾来说，我们就可以看出来，hello驱动代码缺少了核心的硬件层的代码，在古早的内核源码中，我们采取像stm32的一样的写法直接去操作寄存器，但是这样会导致我们的内核源码非常冗余庞大，经过技术进步，现在我们是去操作设备树来进行操控硬件，但是这里，这个简单的led驱动程序我们进行直接操作寄存器来控制led。

​	当然我们无法直接在寄存器的源地址上进行操作，这是非常危险的行为，所以我们需要进行地址映射，来操作相关的寄存器。

```c
/* registers */
//SW_MUX_CTL_PAD_GPIO1_IO03 地址：I20E_0000h base + 68h offset = 20E_0068h
static volatile unsigned int *SW_MUX_CTL_PAD_GPIO1_IO03;

// GPIO5_GDIR 地址：0x020AC004
static volatile unsigned int *GPIO1_GDIR;

//GPIO5_DR 地址：0x020AC000
static volatile unsigned int *GPIO1_DR;
```

​	我们imx6ull板子跑操作系统是自带时钟的，所以不太需要初始化时钟。所以以上三个寄存器就可以完成驱动led。

```c
	/* ioremap */
	// SW_MUX_CTL_PAD_GPIO1_IO03 地址：20E_0000h base + 68h offset = 20E_0068h
	SW_MUX_CTL_PAD_GPIO1_IO03 = ioremap(0X020E0000 + 0x68, 4);
	
	// GPIO1_GDIR 地址：0x020AC004
	GPIO1_GDIR = ioremap(0X0209C004, 4);
	
	//GPIO1_DR 地址：0x020AC000
	GPIO1_DR  = ioremap(0X0209C000, 4);
```

​	这个地址需要去看数据手册找寻地址。imx的数据手册很详细啊，这方面国产芯片做的不如老外太多太多，简直荼毒国内开发者。

​	以上就完成了对寄存器映射，我们就可以去操作寄存器了。

# 3.代码编写

驱动代码：

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

测试代码：

```
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>


// ledtest /dev/myled on
// ledtest /dev/myled off

int main(int argc, char **argv)
{
	int fd;
	char status = 0;
	unsigned int val;
	
	if (argc != 3 && argc != 2)
	{
		printf("Usage: %s <dev> <on|off>\n", argv[0]);
		printf("  eg: %s /dev/myled on\n", argv[0]);
		printf("  eg: %s /dev/myled off\n", argv[0]);
		printf("Usage: %s <dev> \n", argv[0]);
		printf("  eg: %s /dev/myled\n", argv[0]);
		return -1;
	}
	else if(argc == 2)
	{
		fd = open(argv[1], O_RDWR);
		if (fd < 0)
		{
			printf("can not open %s\n", argv[0]);
			return -1;
		}
		read(fd,&val,sizeof(val));
		printf("val = %d\n",val);
	}
	else if(argc == 3)
	{
		// open
		fd = open(argv[1], O_RDWR);
		if (fd < 0)
		{
			printf("can not open %s\n", argv[0]);
			return -1;
		}

		// write
		if (strcmp(argv[2], "on") == 0)
		{
			status = 1;
		}

		write(fd, &status, 1);
	}
	
	return 0;	
}
```

测试结果：

![](D:\学习笔记\驱动学习_重新开始\test2_简单的led驱动程序\截图\测试结果.png)

