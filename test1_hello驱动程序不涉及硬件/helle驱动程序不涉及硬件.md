# **helle驱动程序不涉及硬件**

## 1.什么是驱动程序

### 1.1.驱动的解释

​	编写驱动开始前先了解一下什么是驱动程序？

​	**Linux 驱动是一个让操作系统能够与硬件设备通信的软件组件**。

- **硬件** 只懂得自己的“语言”（寄存器、电气信号）。
- **操作系统（内核）** 希望用统一的“语言”（系统调用、函数接口）来管理所有硬件。
- **驱动** 位于两者之间，它将内核的通用指令“翻译”成硬件能理解的特定命令，同时将硬件的响应“翻译”回内核能理解的信息。

没有驱动，即使硬件已物理连接，内核也无法识别和使用它。

​	Linux 驱动是 **Linux 内核** 的一部分。它运行在 **内核空间**，这是一个拥有最高权限、可以直接访问硬件的特权模式。与之相对的是 **用户空间**，普通应用程序（如浏览器、文本编辑器）就运行在用户空间。

**这种区分带来了两个核心概念：**

1. **内核模块**：驱动通常被编译成 **内核模块**（.ko 文件）。模块可以被动态地加载到运行中的内核，或从内核中卸载，而无需重新启动计算机。这提供了极大的灵活性。
2. **系统调用接口**：用户空间的应用程序不能直接调用内核函数或访问硬件。它们必须通过 **系统调用** 这个受保护的接口来请求内核服务。当应用程序需要操作硬件时（例如，从文件中读取数据），它会发起一个系统调用，内核最终会将这个请求分发给对应的驱动去执行。

### 1.2.驱动的分类

​	Linux 内核将设备分为三大类型，驱动也相应地分为三类：

#### 1.2.1. 字符设备

- **特点**：以 **字节流** 的形式进行顺序访问。通常不支持随机存取。
- **访问方式**：通过设备文件（如 `/dev/ttyS0`， `/dev/input/event0`）进行访问。使用 `open()`, `read()`, `write()`, `close()` 等系统调用。
- **例子**：
  - 键盘、鼠标
  - 串口
  - 打印机
  - 声卡（部分）

#### 1.2.2. 块设备

- **特点**：以 **数据块** 为单位进行访问（如 512字节， 4K字节），支持随机存取。为了提高效率，内核设有复杂的缓存机制。
- **访问方式**：同样通过设备文件（如 `/dev/sda1`， `/dev/nvme0n1p1`）访问。通常通过 `open()`, `read()`, `write()`, `close()` 访问，但底层与字符设备不同。
- **例子**：
  - 硬盘、固态硬盘
  - U盘、SD卡
- **注意**：块设备可以被“挂载”到文件系统目录树上，从而融入文件系统。

#### 1.2.3. 网络设备

- **特点**：面向数据包，而不是字节流。它不对应设备文件，因此没有 `read`/`write` 的概念。
- **访问方式**：通过 **网络接口**（如 `eth0`, `wlan0`）和 **套接字** 接口进行访问。应用程序使用 `socket()`, `bind()`, `send()`, `recv()` 等系统调用。
- **例子**：
  - 以太网卡
  - 无线网卡

## 2.hello驱动程序

### 2.1.资源介绍

1. 正点原子阿尔法开发板
2. 正点原子开发板匹配的内核源码
3. 正点原子提供的资料（如底板原理图）

### 2.2.如何编写hello驱动程序

​	hello驱动程序属于什么？按照上面驱动的分类来说，hello驱动属于字符设备，所以我们需要创造一个字符节点去打开该驱动。

### 2.3.**APP** **打开的文件在内核中如何表示**

​	APP就是用户态。我们使用open函数打开设备节点。

```c
/*函数原型*/
int open(const char *pathname, int flags, mode_t mode);
```

​	open函数他就是完成由用户态到内核态之间的转变，当flags中有创建文件的相关字码时，mode才有效。

![](D:\学习笔记\驱动学习_重新开始\test1_hello驱动程序不涉及硬件\截图\open代码分析.png)

![](D:\学习笔记\驱动学习_重新开始\test1_hello驱动程序不涉及硬件\截图\设备节点复制到内核态.png)

![](D:\学习笔记\驱动学习_重新开始\test1_hello驱动程序不涉及硬件\截图\分配fd.png)

### 2.4.细节补充

```c
struct file {
	union {
		struct llist_node	fu_llist;
		struct rcu_head 	fu_rcuhead;
	} f_u;
	struct path		f_path;
	struct inode		*f_inode;	/* cached value */
	const struct file_operations	*f_op;
	/*
	 * Protects f_ep_links, f_flags.
	 * Must not be taken from IRQ context.
	 */
	spinlock_t		f_lock;
	atomic_long_t		f_count;
	unsigned int 		f_flags;
	fmode_t			f_mode;
	struct mutex		f_pos_lock;
	loff_t			f_pos;
	struct fown_struct	f_owner;
	const struct cred	*f_cred;
	struct file_ra_state	f_ra;

	u64			f_version;
#ifdef CONFIG_SECURITY
	void			*f_security;
#endif
	/* needed for tty driver, and maybe others */
	void			*private_data;

#ifdef CONFIG_EPOLL
	/* Used by fs/eventpoll.c to link all the hooks to this file */
	struct list_head	f_ep_links;
	struct list_head	f_tfile_llink;
#endif /* #ifdef CONFIG_EPOLL */
	struct address_space	*f_mapping;
} __attribute__((aligned(4)));	/* lest something weird decides that 2 is OK */

```

​	上面就是`struct file`结构体的完整内容，每次打开设备节点，或者创建一个文件句柄就会创建一个对应的`struct file`，

```c
unsigned int 		f_flags;
	fmode_t			f_mode;
/*这里面的的f_flags和f_mode就是我们传进来的*/
const struct file_operations	*f_op;
/*这个就是我们定义的open，read......函数的写法*/
static struct file_operations hello_drv = {
	.owner	 = THIS_MODULE,
	.open    = hello_drv_open,
	.read    = hello_drv_read,
	.write   = hello_drv_write,
	.release = hello_drv_close,
};
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
```

​	这里学的时候一直没有搞懂`struct inode	*f_inode;`这个结构体原型非常的长，简单来说，`inode` 表示文件的「身份」，包含文件的所有静态信息（不包含文件名）

​	对于**普通文件**，它描述文件的磁盘位置、大小；

​	对于**设备文件**（如 `/dev/hello`），它描述这个设备在内核中的映射关系（major/minor 对应哪个驱动）。

​	以上的这些信息都是保存在`struct file`里面。

那么内核是如何知道当用户去打开一个设备节点的时候就能找到对应的驱动呢？

```c
major = register_chrdev(0, "hello", &hello_drv);
/*
这个函数会：
向内核的 chrdevs 哈希表注册一个条目；
记录下你的主设备号（major）与文件操作表（&hello_drv）之间的映射；
比如：
chrdevs[major] = &hello_drv;
*/
device_create(hello_class, NULL, MKDEV(major, 0), NULL, "hello");
/*这个函数通过 udev（或 devtmpfs） 在 /dev 下自动创建设备节点 /dev/hello。*/
/*
它做了两件重要的事：
	告诉内核：“有一个名为 hello 的字符设备，它的设备号是 MKDEV(major, 0)”；
	在 /dev 下创建一个对应的设备文件（crw------- 1 root root major, minor /dev/hello）。
*/
//当用户调用 open("/dev/hello") 时：	
sys_open()
  └── do_sys_open()
        └── do_filp_open()
              └── path_openat()
                    └── do_dentry_open()
/*do_dentry_open() 会根据路径 /dev/hello 找到对应的 inode。*/
/*找到设备号就找到需要调用什么驱动了*/
```

## 3.开始写代码

​	才开始写代码就去抄，抄内核的驱动代码。

```tex
1. 确定主设备号，也可以让内核分配
2. 定义自己的 file_operations 结构体
3. 实现对应的 drv_open/drv_read/drv_write 等函数，填入 file_operations 结构体
4. 把 file_operations 结构体告诉内核：register_chrdev
5. 谁来注册驱动程序啊？得有一个入口函数：安装驱动程序时，就会去调用这个入口函数
6. 有入口函数就应该有出口函数：卸载驱动程序时，出口函数调用unregister_chrdev
7. 其他完善：提供设备信息，自动创建设备节点：class_create, device_create
```

最简单的框架：

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
static int __init hello_init(void) 			/*入口函数*/
{
	
}


static void __exit hello_exit(void)			/*出口函数*/
{
	
}
module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
```

简单的驱动程序代码：

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

​	我们可以看到在file_operations所创建的函数中有struct file *file和struct inode *node结构体，这个以后在创建多设备节点时就需要了，因为多设备节点他们的主设备号是一样的，次设备号不一样，可以以此来分别。

​	然后我们再写一个Makefile和测试代码就可以测试了。

![](D:\学习笔记\驱动学习_重新开始\test1_hello驱动程序不涉及硬件\截图\类class.png)

![](D:\学习笔记\驱动学习_重新开始\test1_hello驱动程序不涉及硬件\截图\设备节点.png)

​	可以看到真的创建了hello_class和hello的设备节点

测试现象：

![](D:\学习笔记\驱动学习_重新开始\test1_hello驱动程序不涉及硬件\截图\测试现象.png)