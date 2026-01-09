# 1.led的硬件原理

![](D:\学习笔记\驱动学习_重新开始\test5_led篇\demo1_simple_led\截图\led硬件原理图.png)

​	如图所示，是我的开发板的led原理图，可以看出我的开发板采用的是低电平点亮LED的方式。

# 2.GPIO操作

​	Linux驱动学习操作GPIO与单片机学习类似；

- 有多组 GPIO，每组有多个 GPIO
- 使能：电源/时钟
- 模式(Mode)：引脚可用于 GPIO 或其他功能
- 方向：引脚 Mode 设置为 GPIO 时，可以继续设置它是输出引脚，还是输入引脚
- 数值：
  - 对于输出引脚，可以设置寄存器让它输出高、低电平
  - 对于输入引脚，可以读取寄存器得到引脚的当前电平

# 3.IMX6ULL的GPIO操作

​	在i.MX6ULL上，要完整地使用一个GPIO引脚的功能，主要涉及**三个关键硬件模块**，每个模块内部有各自的寄存器。这三个模块是：

1. **CCM (Clock Controller Module) - 时钟控制模块**
2. **IOMUXC (I/O Multiplex Controller) - IO复用控制器**
3. **GPIO (General Purpose Input/Output) - 通用输入输出控制器本身**

## 3.1. CCM： 提供“动力” - 使能时钟

- **作用**： 就像给一个设备通电。GPIO控制器本身是一个数字电路模块，它需要在时钟信号下才能工作。如果时钟被关闭，你无法读写它的任何寄存器。
- **操作**： 在CCM模块中找到控制对应GPIO组（如GPIO1, GPIO2）时钟的寄存器，将其使能（通常是写1到某个bit）。
- **类比**： 打开一个电器的电源总开关。

## 3.2. IOMUXC： 决定“身份” - 配置引脚复用和电气属性

- **作用**： i.MX6ULL的芯片引脚是“多功能”的，一个物理引脚既可以做GPIO，也可以做UART的TX，或者I2C的SCL等。IOMUXC就是负责配置这个引脚的“身份”和“性格”。
- **操作**：
  - **复用功能 (Mux Mode)**： 设置寄存器，选择这个引脚当前是`GPIO`模式，还是`UART_TX`等其他模式。这是核心步骤。
  - **电气属性 (Pad Settings)**： 配置寄存器的其他位，来设置引脚的驱动强度、上下拉电阻、压摆率、 hysteresis等。这决定了信号的电气特性和抗干扰能力，对稳定性和功耗有影响。
- **类比**： 决定一个多功能房间（引脚）当前是作为**书房（GPIO）** 还是**卧室（UART）** 使用，并且决定房间里的灯光亮度（驱动强度）、门要不要自动上锁（上下拉）。

## 3.3. GPIO： 执行“动作” - 控制输入/输出和电平

- **作用**： 当引脚被设置为GPIO身份后，就由GPIO模块的寄存器来具体控制它的行为。
- **核心寄存器**：
  - **GDIR (方向寄存器)**： 设置该引脚为`输入(0)`还是`输出(1)`。
  - **DR (数据寄存器)**：
    - 当引脚是**输出**时：向对应的位写`1`或`0`，使引脚输出高或低电平。
    - 当引脚是**输入**时：读取对应的位，获取当前引脚的电平状态。
  - **PSR (引脚状态寄存器)**： 专门用于读取引脚当前的实际电平（输入时更准确）。
  - **ICR1/ICR2 (中断配置寄存器)** 等： 配置中断触发方式（边沿/电平）。

# 4.代码演示

​	上面的理论讲完了，下面就该写代码了。接下来是代码演示。

## 4.1.代码框架

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
static ssize_t hello_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	
	return 0;
}
static ssize_t hello_drv_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	
	return 0;
}
static int hello_drv_open (struct inode *node, struct file *file)
{
	
	return 0;
}

static int hello_drv_close (struct inode *node, struct file *file)
{
	
	return 0;
}                                       */
static struct file_operations hello_drv = {
	.owner	 = THIS_MODULE,
	.open    = hello_drv_open,
	.read    = hello_drv_read,
	.write   = hello_drv_write,
	.release = hello_drv_close,
};

static int __init hello_init(void)
{
	return 0;
}
static void __exit hello_exit(void)
{
}
module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
```

​	非常干净的代码框架，来自hello程序，之前的hello写错了，之前一直发现没法打印出字符到终端需要使用，`dmesg`来查看日志才能看到打印的信息，是因为，我没有创建主设备和class，device节点。

## 4.2编写驱动程序的套路：

- **确定主设备号，也可以让内核分配**
- **定义自己的 file_operations 结构体**
- **实现对应的 drv_open/drv_read/drv_write 等函数，填入 file_operations 结构体**
- **把 file_operations 结构体告诉内核：register_chrdev**
- **谁来注册驱动程序啊？得有一个入口函数：安装驱动程序时，就会去调用这个入口函数**
- **有入口函数就应该有出口函数：卸载驱动程序时，出口函数调用unregister_chrdev**
- **其他完善：提供设备信息，自动创建设备节点：class_create, device_create**

## 4.3.驱动怎么操作硬件？

​	**通过 ioremap 映射寄存器的物理地址得到虚拟地址，读写虚拟地址**

## 4.4.驱动怎么和 APP 传输数据？

​	**通过 copy_to_user、copy_from_user 这 2 个函数。**

## 4.5.拆解代码

### 4.5.1.打通用户空间应用程序到内核驱动程序再到硬件设备的完整通路

​	写这个驱动需要明白我们的核心目的是什么？

​	**我们需要从用户空间应用程序到内核驱动程序再到硬件设备的完整通路。**

​	在Linux驱动开发中，`register_chrdev`、`class_create`、`device_create` 这三个函数是**字符设备驱动注册和创建设备节点的核心步骤**。

```
register_chrdev()    # 1. 注册驱动到内核
     ↓
class_create()       # 2. 创建设备类（sysfs）
     ↓
device_create()      # 3. 创建设备节点（/dev/led）
```

```c
/*  确定主设备号 */
static int major = 0;
/*  定义class */
static struct  class *led_class;
static int __init led_init(void)
{   
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
```

### 4.5.2.操作寄存器

​	前面讲了，使用`ioremap` ，映射寄存器的物理地址得到虚拟地址。所以我们需要知道寄存器的地址。

完整的代码：

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
```

## 4.6.测试结果

​	就是成功的点亮和关闭了led，手机拍出来效果不好，led关闭的时候还有点微弱的红灯，不知道是为什么，应该是硬件问题。