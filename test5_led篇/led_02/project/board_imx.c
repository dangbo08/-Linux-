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
//static volatile unsigned int *GPIO1_GDIR = NULL;
//209_C000 = 0x0209C000
//static volatile unsigned int *GPIO1_DR = NULL;
GPIOx_Func __iomem *GPIO1_Func;



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
            /*  struct imx6ull_gpio __iomem *gpio =
    ioremap(GPIO1_BASE_ADDR, GPIO_SIZE); */
            GPIO1_Func = ioremap(GPIO1_BASEADDR,  GPIO_REG_SIZE); 
            if (!GPIO1_Func)
            {
                printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
                return -ENOMEM;
            }
            
			//imx6ull_gpio_opr->map_virtual((void**)&GPIO1_GDIR,GPIO1_GDIR_ADDR);
			//imx6ull_gpio_opr->map_virtual((void**)&GPIO1_DR,GPIO1_DR_ADDR);
		}
		imx6ull_gpio_opr->reuse((void*)IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03,5,0,4);
		imx6ull_gpio_opr->direction_out_int((void*)GPIO1_Func->GDIR,1,3);
		imx6ull_gpio_opr->default_direc((void*)GPIO1_Func->DR,0,3);
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
			imx6ull_gpio_opr->write_gpio((void*)GPIO1_Func->DR,0,3);
		}
		else /*off:output 1*/
		{
			printk("status == 0 \n");
			imx6ull_gpio_opr->write_gpio((void*)GPIO1_Func->DR,1,3);
		}
	}
	return 0;
}

static int imx6ull_gpio_read(int which)
{
	if(which == 0)
	{
		return imx6ull_gpio_opr->read_gpio((void*)GPIO1_Func->DR,3);
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
    //iounmap
    iounmap(GPIO1_Func);
	//imx6ull_gpio_opr->release_map((void**)&GPIO1_DR);
	//imx6ull_gpio_opr->release_map((void**)&GPIO1_GDIR);
	imx6ull_gpio_opr->release_map((void**)&IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03);
	imx6ull_gpio_opr->release_map((void**)&CCM_CCGR1);
}