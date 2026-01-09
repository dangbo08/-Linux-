
#include "car.h"
/* 导出的链表头 */
LIST_HEAD(carlist);   // 注意必须是全局变量！才能导出！

EXPORT_SYMBOL(carlist);
static int __init list_demo_init(void)
{
    //INIT_LIST_HEAD(&carlist);
    struct car *bmwcar = kzalloc(sizeof(struct car),GFP_KERNEL);
    if (!bmwcar)
        return -ENOMEM;
    //初始化内部的list
    INIT_LIST_HEAD(&bmwcar->list);
    strcpy(bmwcar->carname , "bmw");
    strcpy(bmwcar->carmodel , "m4");
    bmwcar->max_speed = 1000;
    list_add(&bmwcar->list,&carlist);
    struct car *acar; //计数器
    list_for_each_entry(acar, &carlist, list)
    {
        printk(KERN_INFO "carname = %s,carmodel = %s,max_speed = %d",acar->carname,acar->carmodel,acar->max_speed);
    }

    pr_info("hello_kernel_world\n");
    
    return 0;
}
static void __exit list_demo_exit(void)
{
    /* 在 exit 里面释放链表节点 */
    struct car *acar, *tmp;  
    list_for_each_entry_safe(acar, tmp, &carlist, list) {
        list_del(&acar->list);
        kfree(acar);
    }
    pr_info("goodbye kernel_world\n");
}
module_init(list_demo_init);
module_exit(list_demo_exit);
MODULE_AUTHOR("DB");
MODULE_LICENSE("GPL");



