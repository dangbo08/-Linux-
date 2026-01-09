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
#include "led_drv.h"
static int major;
static struct class *led_class;
struct led_operations *s_led_opr;

/* 完善 file_operations*/
/*
*struct file *file：和open是同一个
*   可以从 file->private_data 取数据
*char __user *buf：这是用户态的指针
*   不能直接*buf=x，而需要使用：copy_to_user(buf, kernel_buf, n);
*size_t size：用户希望读多少字节：read(fd, buf, 100); size=100
*loff_t *offset：文件偏移
 */
static ssize_t led_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}
/*
*struct file *file：和open是同一个
*   可以从 file->private_data 取数据
*char __user *buf：这是用户态的指针
*   不能直接*buf=x，而需要使用：copy_from_user(buf, kernel_buf, n);
*size_t size：用户希望写多少字节：write(fd, buf, 100); size=100
*loff_t *offset：文件偏移
 */
static ssize_t led_drv_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    char resbuf;
    struct inode *inode = file_inode(file);
    int minor = iminor(inode);
    int err;
    err = copy_from_user(&resbuf,buf,1);
    s_led_opr->ctl(minor,resbuf);
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}
/*
*struct inode *node：设备本身的节点
*   iminor(inode)->次设备号     imajor(inode)->主设备号
*struct file *file：代表这一次的打开行为，
*   一个设备可以被打开多次，每一次open都对应一个struct file
*/
static int led_drv_open (struct inode *node, struct file *file)
{
    int minor = iminor(node);
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    /* 当我open节点的时候就应该初始化了 */
    s_led_opr->init(minor);
	return 0;
}
/*
*struct inode *node：设备本身的节点
*   iminor(inode)->次设备号     imajor(inode)->主设备号
*struct file *file：代表这一次的打开行为，
*   一个设备可以被打开多次，每一次close都对应一个struct file
*/
static int led_drv_close (struct inode *node, struct file *file)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

static struct file_operations led_drv = {
    .owner      =       THIS_MODULE,
    .open       =       led_drv_open,
    .read       =       led_drv_read,
    .write      =       led_drv_write,
    .release    =       led_drv_close,
};
void led_device_create(int minor)
{
    /*
    struct device *device_create(struct class *cls, struct device *parent,
			     dev_t devt, void *drvdata,
			     const char *fmt, ...);
    */
    /*
    *struct class *cls:这个设备属于哪个“设备类” ;class 会在 /sys/class/ 下生成目录
    *   eg:
    *       /sys/class/db_led_class/
    *           db_led0
    *           db_led1
    * class 是“对用户可见的设备分类”
    * struct device *parent:父设备，这个设备被挂在哪个设备下的，用于设备树/总线层次结构，这里没有用NULL
    * dev_t devt：MKDEV(major, minor)，设备号（主+次），主设备号：找驱动，次设备号：区分同类设备
    *   内核通过/dev/db_led0，将其映射到led_drv.file_operations上
    * void *drvdata：驱动私有数据
    * const char *fmt, ...：设备节点名字
    */
    device_create(led_class,NULL,MKDEV(major,minor),NULL,"db_led%d",minor);
}
void led_device_remove(int minor)
{
    device_destroy(led_class, MKDEV(major, minor));
}
/* 拿到led的信息 */
void register_led_operations(struct led_operations *opr)
{
    s_led_opr = opr;
}
EXPORT_SYMBOL(led_device_create);
EXPORT_SYMBOL(led_device_remove);
EXPORT_SYMBOL(register_led_operations);
static int __init led_drv_init(void)
{
    /*
     注册字符设备，并获得主设备号 
    static inline int register_chrdev(unsigned int major, const char *name,
				  const struct file_operations *fops)
    */
    major = register_chrdev(0,"db_led",&led_drv);
    /* 创建类目录，用于用户空间管理 */
    /*
    #define class_create(owner, name)		\
    ({						\
        static struct lock_class_key __key;	\
        __class_create(owner, name, &__key);	\
    })
    */
    led_class = class_create(THIS_MODULE,"led_class");
    /* 这里本来需要封装设备节点的创建，但是设备节点创造多少由platform_driver提供，所以封装到外部 */
    return 0;
}

static void __exit led_drv_exit(void)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    class_destroy(led_class);
    unregister_chrdev(major,"db_led");

}


module_init(led_drv_init);
module_exit(led_drv_exit);

MODULE_LICENSE("GPL");