# Linux内核基础学习

# 0.前言

​	Linux内核的组成就不说了，基本上不管是韦东山还是正点原子都会讲一遍，查资料很快就能查出来，写笔记占的篇幅太多了，到时候看又不想看，还是不记笔记的好，下面直接开始干活总结。本来都学到后面了，但是笔记记在书上了，很多代码又没办法往书上写，所以记录一下这一篇。

# 1.设备驱动程序的基础

​	驱动程序是专用于控制和管理特定硬件设备的软件，因此也被称为设备驱动程序。这段话就总结了驱动程序的作用，而我们为什么学习驱动程序？是因为在嵌入式作业环境中我们需要控制不同的硬件设备，与stm32控制硬件设备的不同点是，我们使用了Linux系统，stm32是裸机开发开发，我们通过直接控制寄存器达到控制硬件设备的需求；但是Linux中不存在这种现象，为什么呢？因为Linux系统分为了用户态和内核态。

## 1.1.用户态和内核态

![](D:\学习笔记\驱动学习_重新开始\test4_基础学习\截图\用户空间和内核空间.png)

​	用户态和内核态的核心区别就是内存空间的访问。

| 区域                        | 访问者                        | 内容                                               |
| --------------------------- | ----------------------------- | -------------------------------------------------- |
| **用户空间** (User Space)   | 用户态程序（包括 root）可访问 | 程序代码、库、用户栈、堆、文件映射等               |
| **内核空间** (Kernel Space) | 只有内核态可访问              | 内核代码、内核数据、内核栈、页表、驱动、硬件映射等 |

​	学到这里我有疑惑，为什么root用户能够读取到内核的一些值呢？root用户能够访问内核空间吗？答案是不能的。
​	用户态能访问的内核内容 = “内核主动映射出来”的**虚拟区域**。

​	但是当我听课的时候又有用户态到内核态的转换，这是为什么呢？

​	**root 如何获得真正的内核态能力？**

​	**A. 加载内核模块（最常见、合法）**

```sh
insmod mymod.ko
```

​	**B. 利用内核漏洞（攻击方式）**

用户态和内核态就是这些内容了。

## 1.2.模块的概念

```
CONFIG_MODULES=y
```

​	这个模块化的概念是我在做4G图传时选择平台出现的问题记忆比价深刻，当时4G网卡的驱动版本太低了，但是他是内置的模块无法改变，后面查阅资料看到了单独进行模块化，`CONFIG_MODULES=y`这个是表示Linux系统允许模块化的存在，`CONFIG_MODULES=n`如果是这样就不允许模块化的存在所有驱动必须内置

```
CONFIG_USB_ETH=y   # 这个驱动内建
CONFIG_I2C_DEV=m   # 这个驱动编译为模块
CONFIG_SPI_MASTER=y
CONFIG_BT_HCI=m
```

我们构建Linux内核的时候有一个.config文件里面就有上面那些内容，将CONFIG_XXX_XXX=m就是将这个单独的驱动模块化，当时我就是这么操作我的4G模块驱动的，但是还是没有成功。这是题外话了，这个和后面的makefile文件有异曲同工之妙，后面讲。

## 1.3.模块的卸载与加载

​	模块是什么？就是.ko文件，我们经常用的加载模块和卸载模块的是什么命令？

```sh
insmod xxx.ko		# 加载模块
rmmod  xxx.ko		# 卸载模块
```

​	写好驱动以后直接insmod，需要卸载直接rmmod，方便快捷。

​	但是其实还有一个命令可以加载驱动那就是modprobe命令。这个命令跟insmod就不一样了，他需要将我们编译好的驱动放置在内核的modules目录下：/lib/modules/$(uname -r)/，然后还需要depmod一下；depmod生成模块的依赖信息，然后就能使用modprobe了。

​	**depmod：**感觉这个可以讲一下，简单讲一下这个作业是什么；**依赖信息**重要的就是这四个字，比如说我有三个模块，a.ko,b.ko,c.ko我想要加载a.ko,但是加载他的前提是b.ko和c.ko都是加载的，我们使用insmod a.ko就会报错，必须先insmod b.ko c.ko才能insmod a.ko但是使用depmod的区别就是他会去生成这些依赖信息，当我三个模块都没有被加载的时候，只需要使用modprobe a.ko 他就会同时加载这三个ko文件。

​	卸载命令就是：modprobe -r a.ko ，同理b.ko和c.ko也会被卸载

​	引用计数记录模块的使用次数，上面讲的有点不对；

​	比如modprobe a.ko b.ko和c.ko会被启动，他们的计数器就会+1，当出现一个d.ko 他的依赖也是b.ko和c.ko那么modprobe d.ko 的时候b.ko和c.ko的计数器就会再一次+1，当我们modprobe -r a.ko的时候b.ko和c.ko的计数器就会-1，当refcount=0的时候表示b和c模块都没有被引用了，但是没有被卸载。

​	**modprobe 会自动加载依赖模块，但不会自动卸载依赖模块。refcount 归零也不会自动卸载模块。**

## 1.4.驱动程序的框架

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
static int __init hello_kernel_world(void)
{
    pr_info("hello_kernel_world\n");
    return 0;
}
static void __exit hello_kernel_world(void)
{
    pr_info("goodbye kernel_world\n");
}
module_init(hello_kernel_world);
module_exit(hello_kernel_world);
MODULE_AUTHOR("DB");
MODULE_LICENSE("GPL");
```

​	以上就是驱动程序的框架，可以看到同样作为C语言编写的代码他并没有使用main函数来作为入点然后使用return来结束工程，而是使用module_init来修饰函数入点，module_exit修饰函数出点，hello_kernel_world这个函数可以被随意命名。使用modprobe 和insmod的时候就会加载module_init，被卸载的时候就会加载module_exit。

​	模块信息和许可证不详细讲了，现在不是很重要，理论的东西看太多会很枯燥的。

## 1.5.错误信息

​	这个部分内容很长啊

| 错误码 | 名称                 | 中文说明                                 |
| ------ | -------------------- | ---------------------------------------- |
| **1**  | EPERM                | 操作不被允许（通常是权限不足导致）       |
| **2**  | ENOENT               | 文件或目录不存在                         |
| **3**  | ESRCH                | 没有对应的进程                           |
| **4**  | EINTR                | 系统调用被中断（信号中断）               |
| **5**  | EIO                  | I/O 输入输出错误                         |
| **6**  | ENXIO                | 设备或地址不存在                         |
| **7**  | E2BIG                | 参数列表过长（exec 参数过大）            |
| **8**  | ENOEXEC              | 可执行文件格式错误（例如魔数不对）       |
| **9**  | EBADF                | 文件描述符无效                           |
| **10** | ECHILD               | 无子进程                                 |
| **11** | EAGAIN / EWOULDBLOCK | 资源暂时不可用（常见于非阻塞 I/O）       |
| **12** | ENOMEM               | 内存不足                                 |
| **13** | EACCES               | 权限不足（拒绝访问）                     |
| **14** | EFAULT               | 无效的地址（非法内存访问）               |
| **15** | ENOTBLK              | 需要块设备但给的是其他类型               |
| **16** | EBUSY                | 设备或资源忙                             |
| **17** | EEXIST               | 文件已存在                               |
| **18** | EXDEV                | 跨设备链接错误（不同挂载点之间 ln 失败） |
| **19** | ENODEV               | 设备不存在                               |
| **20** | ENOTDIR              | 不是目录                                 |
| **21** | EISDIR               | 是一个目录（不能作为文件操作）           |
| **22** | EINVAL               | 参数无效（最常见错误之一）               |
| **23** | ENFILE               | 系统文件打开数已达上限                   |
| **24** | EMFILE               | 进程打开的文件数已达上限                 |
| **25** | ENOTTY               | 不适当的 ioctl 操作（设备不支持）        |
| **26** | ETXTBSY              | 文本文件繁忙（执行时不能写入）           |
| **27** | EFBIG                | 文件过大（超出系统支持的大小）           |
| **28** | ENOSPC               | 磁盘空间不足                             |
| **29** | ESPIPE               | 非法的 seek 操作（例如对管道 seek）      |
| **30** | EROFS                | 文件系统为只读                           |
| **31** | EMLINK               | 链接数量过多                             |
| **32** | EPIPE                | 管道破裂（对端关闭写管道时写入）         |
| **33** | EDOM                 | 数学参数超出定义域                       |
| **34** | ERANGE               | 数学结果超出范围                         |

在errno-base.h中有34个基础错误集，在errno.h中涵盖了所有错误集，使用：`man errno`查看。

**处理空指针错误**：

​	使用三个函数：ERR_PTR,IS_ERR,PTR_ERR;这三个函数，什么时候使用：

当内核中函数的返回值为指针的时候使用：

```c
void * demo(void)
{
    if(xxxxx)
    {
        return NULL;
    }
    else
    {
        /*比如说这里我需要返回一个错误码*/
        /*直接这样return ENOENT;肯定是不行的*/
        /*使用ERR_PTR将其转变为错误指针*/
        return ERR_PTR(ENOENT);
    }
}


int xxx(void)
{
    void*demo1 = demo();
    if(IS_ERR(demo1))   /*使用IS_ERR检测他是不是错误指针*/
    {
        PTR_ERR(demo1); /*是错误指针就使用PTR_ERR还原他*/
	}
}
```

## 1.6.消息打印

​	这就是消息打印的位置，所有的内容都会输出到日志缓冲区，但是有一些内容会输出到终端，这个就是跟消息打印的级别有关系了，感兴趣可以看一下，我反正没成功输出到终端，不重要，不影响下面的操作，我没有花费太多时间。

## 1.7.makefile文件

```makefile
db@db:~/vm_driver/01$ cat Makefile 
# 使用系统默认的编译器，不再设置 ARCH 和 CROSS_COMPILE
KERN_DIR := /lib/modules/$(shell uname -r)/build
CURRENT_PATH := $(shell pwd)
CODE := $(CURRENT_PATH)/drv
obj-m += drv/

all:
	make -C $(KERN_DIR) M=$(CURRENT_PATH) modules
	mv $(CODE)/*.o $(CURRENT_PATH)/build/obj
	mv $(CODE)/*.ko $(CURRENT_PATH)/build/ko
clean:
	make -C $(KERN_DIR) M=$(CURRENT_PATH) clean
```

​	这个obj-m就是编译成模块的意思，obj-y就是内置到Linux内核了。drv/就是代码所在的文件夹

```makefile
db@db:~/vm_driver/01$ cat drv/Makefile 
obj-m += hello_kernel_world.o
```

在这个文件夹下又要写makefile文件。
