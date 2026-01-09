# 1.对比直接驱动led模块

​	上一讲中我们操作gpio的引脚电平来驱动led的亮灭，我们发现他非常简单的去修改了led的寄存器，然后将他全部写在了驱动程序里面，如果我们需要重新操作另外一个GPIO或者操作另外一个板子的led模块，我们就需要重新另外写代码。这就是Linux内核2.x版本以前的设计方式，导致了当时的Linux内核非常庞大冗余，如何更改？第一步就是抽出他们的相同部分进行封装，然后需要修改就只需要提供底板程序就行了。

# 2.分层分析

## 2.1.GPIO操作

​	GPIO操作需要做什么？需要操作寄存器。有关寄存器的操作就可以抽出来

```c
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
```

```c
#include "led_gpio.h"

/*
struct gpio_operations {
	int (*map_virtual)(void*WHICH_GPIO,unsigned int GPIO_ADDR);
	int (*reuse)(void*WHICH_GPIO,unsigned int WHICh_MODE);
	int (*direction)(void*WHICH_GPIO,unsigned int INIT_DIRE);
	int (*read_gpio)(void*WHICH_GPIO);  
	int (*write_gpio)(void*WHICH_GPIO,unsigned int VAL,unsigned int gpio_pin);
	int (*release_map)(void*WHICH_GPIO);
};

*/

static int gpio_map_virtual(void**WHICH_GPIO,unsigned int GPIO_ADDR)
{
	*WHICH_GPIO = ioremap(GPIO_ADDR, 4);
    if (!*WHICH_GPIO)
        return -ENOMEM;  // 映射失败

    return 0;  // 成功
}

static int gpio_reuse(void*WHICH_GPIO,unsigned int WHICH_MODE,unsigned int gpio_pin,unsigned int bits_ctl)
{
	volatile unsigned int *reg = (volatile unsigned int *)WHICH_GPIO;
	//定义一个变量取出原始值
	unsigned int val;
	unsigned int i_mask;
	i_mask = ((1U << bits_ctl) - 1) << gpio_pin;
	val = *reg;
	//清零
	val &= ~(i_mask);

	val |= ( (WHICH_MODE << gpio_pin) & i_mask );
	*reg = val;
	return 0;
}

static int gpio_direction(void*WHICH_GPIO,unsigned int OUT_INT_PUT,unsigned int gpio_pin)
{
	volatile unsigned int *reg = (volatile unsigned int *)WHICH_GPIO;
	if(OUT_INT_PUT == 1)
	{
		*reg |= (1 << gpio_pin);
	}
	else
		*reg &= ~(1 << gpio_pin);
	return 0;
}
static int gpio_default_direc(void*WHICH_GPIO,unsigned int default_dire,unsigned int gpio_pin)
{
	volatile unsigned int *reg = (volatile unsigned int *)WHICH_GPIO;
	if(default_dire == 1)
	{
		*reg |= (1 << gpio_pin);
	}
	else
		*reg &= ~(1 << gpio_pin);
	return 0;
}

static int gpio_read_pin(void*WHICH_GPIO,unsigned int gpio_pin)
{
	volatile unsigned int *reg = (volatile unsigned int *)WHICH_GPIO;
	return ((*reg >> gpio_pin) & 0x1);
}

static int gpio_write_pin(void*WHICH_GPIO,unsigned int VAL,unsigned int gpio_pin)
{
	volatile unsigned int *reg = (volatile unsigned int *)WHICH_GPIO;
	if(VAL == 1)
	{
		*reg |= (1 << gpio_pin);
	}
	else
		*reg &= ~(1 << gpio_pin);
	return 0;
}

static int gpio_release_map(void**WHICH_GPIO)
{
	if (*WHICH_GPIO) {
        iounmap(*WHICH_GPIO);
        *WHICH_GPIO = NULL;  // 这时外部指针才会被置 NULL
    }
    return 0;
}


static struct gpio_operations gpio_opr = {
	.map_virtual 			= 	gpio_map_virtual,
	.reuse 					= 	gpio_reuse,
	.direction_out_int 		= 	gpio_direction,
	.default_direc			= 	gpio_default_direc,
	.read_gpio				= 	gpio_read_pin,
	.write_gpio				=	gpio_write_pin,
	.release_map			= 	gpio_release_map,
};

struct gpio_operations *get_gpio_opr(void)
{
	return &gpio_opr;
}
```

## 2.2.硬件层

```c
#ifndef _BOARD_IMX_H
#define _BOARD_IMX_H
#include "led_gpio.h"
struct led_operations {
	int num;                                            //次设备号的数值    也就是需要几个led灯
	int (*init)(int which);							    //init--初始化LED，	which--哪一个LED
	int (*ctl)(int wshich , char status);			    //ctl--控制LED，	which--哪一个LED，status--状态--0-灭/1-亮
	int (*read)(int wshich);							//red--读取LED电平，	which--哪一个LED
};

struct led_operations *get_board_led_opr(void);
void delete_io(void);
#endif
```

```c
#include "board_imx.h"
static struct gpio_operations *imx6ull_gpio_opr = NULL;
#define CCM_CCGR1_ADDR								0X020C406C
#define IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03_ADDR		0X020E0068
#define GPIO1_GDIR_ADDR								0X0209C004
#define GPIO1_DR_ADDR								0X0209C000

//Address: 20C_4000h base + 6Ch offset = 0X020C_406Ch
static volatile unsigned int *CCM_CCGR1 = NULL;
//Address: 20E_0000h base + 68h offset = 0X020E_0068h
static volatile unsigned int *IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03 = NULL;
//209_C004 = 0x0209C004
static volatile unsigned int *GPIO1_GDIR = NULL;
//209_C000 = 0x0209C000
static volatile unsigned int *GPIO1_DR = NULL;
/*硬件初始化*/
static int imx6ull_gpio_init(int which) /* 初始化 LED, which-哪个 LED */
{
	if(which == 0)
	{
		if(!CCM_CCGR1)
		{
			imx6ull_gpio_opr = get_gpio_opr();
			imx6ull_gpio_opr->map_virtual((void**)&CCM_CCGR1,CCM_CCGR1_ADDR);
			imx6ull_gpio_opr->map_virtual((void**)&IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03,IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03_ADDR);
			imx6ull_gpio_opr->map_virtual((void**)&GPIO1_GDIR,GPIO1_GDIR_ADDR);
			imx6ull_gpio_opr->map_virtual((void**)&GPIO1_DR,GPIO1_DR_ADDR);
		}
		imx6ull_gpio_opr->reuse((void*)IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03,5,0,4);
		imx6ull_gpio_opr->direction_out_int((void*)GPIO1_GDIR,1,3);
		imx6ull_gpio_opr->default_direc((void*)GPIO1_DR,0,3);
	}
	return 0;
}
/*映射所有寄存器*/
static int imx6ull_gpio_ctl(int which,char status)
{
	if(which == 0)
	{
		if(status == '1') /*on:output 0*/
		{
			printk("status == 1 \n");
			imx6ull_gpio_opr->write_gpio((void*)GPIO1_DR,0,3);
		}
		else /*off:output 1*/
		{
			printk("status == 0 \n");
			imx6ull_gpio_opr->write_gpio((void*)GPIO1_DR,1,3);
		}
	}
	return 0;
}

static int imx6ull_gpio_read(int which)
{
	if(which == 0)
	{
		return imx6ull_gpio_opr->read_gpio((void*)GPIO1_DR,3);
	}
	return -1;
}
static struct led_operations board_demo_led_opr = {
	.num 	= 	1,
	.init 	= 	imx6ull_gpio_init,
	.ctl 	= 	imx6ull_gpio_ctl,
	.read  	=	imx6ull_gpio_read,
};
struct led_operations *get_board_led_opr(void)
{
	return &board_demo_led_opr;
}
void delete_io(void)
{  
	imx6ull_gpio_opr->release_map((void**)&GPIO1_DR);
	imx6ull_gpio_opr->release_map((void**)&GPIO1_GDIR);
	imx6ull_gpio_opr->release_map((void**)&IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03);
	imx6ull_gpio_opr->release_map((void**)&CCM_CCGR1);
}
```

## 2.3.驱动模块层

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
#include "board_imx.h"
/* 1. 确定主设备号 */
static int major;
static struct class *led_class;
struct led_operations *p_led_opr;

/* 3. 实现对应的open/read/write等函数，填入file_operations结构体                   */
static ssize_t led_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	
	struct inode *inode = file_inode(file);
	int minor = iminor(inode);
	int str = p_led_opr->read(minor);
	
	if (copy_to_user(buf, &str,sizeof(str)))
   		return -EFAULT;
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return sizeof(str);
}

static ssize_t led_drv_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	int err;
	char status;
	struct inode *inode = file_inode(file);
	int minor = iminor(inode);
	printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
	err = copy_from_user(&status,buf,1);
	p_led_opr->ctl(minor,status);

	return 1;
}

static int led_drv_open (struct inode *node, struct file *file)
{
	int minor = iminor(node);
	printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
	p_led_opr->init(minor);
	return 0;
}

static int led_drv_close (struct inode *node, struct file *file)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

/* 2. 定义自己的f  ile_operations结构体                                              */
static struct file_operations led_drv = {
	.owner	 = THIS_MODULE,
	.open    = led_drv_open,
	.read    = led_drv_read,
	.write   = led_drv_write,
	.release = led_drv_close,
};

/* 4. 把file_operations结构体告诉内核：注册驱动程序                                */
/* 5. 谁来注册驱动程序  啊？得有一个入口函数：安装驱动程序时，就会去调用这个入口函数 */
static int __init led_init(void)
{
	int err;
	int i;
	printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
	major = register_chrdev(0,"db_led",&led_drv);

	led_class = class_create(THIS_MODULE,"db_led_class");
	err = PTR_ERR(led_class);
	if(IS_ERR(led_class)){
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		unregister_chrdev(major,"db_led");
		return -1;
	}
	p_led_opr =   get_board_led_opr();
	for(i = 0;i < p_led_opr->num;i++)
		device_create(led_class,NULL,MKDEV(major,i),NULL,"db_led%d",i);
	return 0;
}

/* 6. 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数           */
static void __exit led_exit(void)
{
	int i;
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	delete_io();
	for(i = 0;i < p_led_opr->num;i++)
		device_destroy(led_class, MKDEV(major, i)); /* /dev/100ask_led0,1,... */
	class_destroy(led_class);
	unregister_chrdev(major, "db_led");
}


/* 7. 其他完善：提供设备信息，自动创建设备节点                                     */

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
```

# 3.知识点补充

​	上面的代码就能实现简单的驱动分层，用于入门阶段。下面针对上面的代码进行知识点补充，主要是函数的知识点。

## 3.1.**ioremap** **函数的使用**

​	**`ioremap`函数的作用：**把“物理地址”映射成“内核可访问的虚拟地址”。

​	CPU 无法直接访问物理地址（尤其是 ARM 的高端口寄存器），必须先映射到虚拟地址空间，才能用指针去读写。

函数原型：

```c
void __iomem *ioremap(phys_addr_t phys_addr, size_t size);
/*头文件*/
#include <asm/io.h>
/*
*phys_addr：要映射的物理地址（寄存器的地址）
*size：映射的大小（字节数）
*/
/*
*返回值：
*成功：虚拟地址（但带有 __iomem 标记）
*失败：NULL
*/
```

​	这里我发现了韦东山课程中和我查到的资料有所不同

```c
static int gpio_map_virtual(void**WHICH_GPIO,unsigned int GPIO_ADDR)
{
	*WHICH_GPIO = ioremap(GPIO_ADDR, 4);
    if (!*WHICH_GPIO)
        return -ENOMEM;  // 映射失败

    return 0;  // 成功
}
```

​	就是这个：`*WHICH_GPIO = ioremap(GPIO_ADDR, 4);`，我查到的资料这里映射的虚拟地址的大小为4byte，并非4KB。如果需要映射4KB需要：*WHICH_GPIO = ioremap(GPIO_ADDR, 0X1000);

​	还有查看数据手册，将寄存器封装进结构体中，结构体成员顺序必须和芯片手册完全一致，这样结构体只需要映射一次。

