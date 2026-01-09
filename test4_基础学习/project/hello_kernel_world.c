#include "car.h"
MODULE_LICENSE("GPL");

/* 引入 provider.c 导出的链表头 */
extern struct list_head carlist;

static int __init consumer_init(void)
{
    struct car *acar;

    printk("hello: start to read carlist...\n");

    list_for_each_entry(acar, &carlist, list) {
        printk("hello: carname=%s  carmodel=%s speed=%d\n", acar->carname,acar->carmodel, acar->max_speed);
    }

    return 0;
}

static void __exit consumer_exit(void)
{
    printk(KERN_INFO "consumer: exit\n");
}

module_init(consumer_init);
module_exit(consumer_exit);