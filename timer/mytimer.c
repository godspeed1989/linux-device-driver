#include <linux/module.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <asm/param.h>

MODULE_LICENSE("GPL");

int cnt;
struct timer_list mytimer;

static void time_up(unsigned long data)
{
	cnt++;
	printk("mytimer: <%d> time is up.\n",cnt);
	mytimer.expires = jiffies + HZ;
	add_timer(&mytimer);
}

int __init timer_module_init(void)
{
	cnt = 0;

	init_timer(&mytimer);
	mytimer.function = time_up;
	mytimer.data = 0;
	mytimer.expires = jiffies + HZ;
	add_timer(&mytimer);

	printk("mytimer: timer_module_init()\n");
	return 0;
}
module_init(timer_module_init);

void __exit timer_module_exit(void)
{
	del_timer(&mytimer);
	printk("mytimer: timer_module_exit()\n");
}
module_exit(timer_module_exit);
