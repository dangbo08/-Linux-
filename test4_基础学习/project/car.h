#ifndef _CAR_H
#define _CAR_H
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>       // kzalloc, kfree
#include <linux/list.h>       // list_head, list functions
#include <linux/string.h>     // strcpy

struct car {
    char carname[20];
    char carmodel[20];
    int max_speed;
    struct list_head list;
};


#endif
