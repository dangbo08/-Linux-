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