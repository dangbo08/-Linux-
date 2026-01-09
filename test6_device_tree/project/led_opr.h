#ifndef _LED_OPR_H
#define _LED_OPR_H
/*
static struct led_resource* led_board_A;
void set_led_resource(struct led_resource* led_resource);
*/
struct led_operations {
	int (*init) (void *data,int which); /* 初始化LED, which-哪个LED */       
	int (*ctl) (void *data,int which, char status); /* 控制LED, which-哪个LED, status:1-亮,0-灭 */
	void *data;
};
/*
static struct led_operations* led_opr_board;
void set_led_opr(struct led_operations*led_opr);
*/

struct led_operations *get_board_led_opr(void);

#endif