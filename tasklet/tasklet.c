#include <linux/module.h>
#include <linux/interrupt.h>

void tasklet_action(unsigned long t)
{
	printk("tasklet: tasklet action()\n");
}

/*can be replaced by DECLARE_TASKLET*/
struct tasklet_struct my_tasklet = {
	.next = NULL,
	.state = 0,
	.count = ATOMIC_INIT(0),
	.func = tasklet_action,
	.data = 0,
};

void do_work_uh(void)
{
	printk("tasklet: do_work_uh()\n");
	tasklet_schedule(&my_tasklet);
}

int __init tasklet_module_init(void)
{
	printk("tasklet: tasklet_module_init()\n");
	do_work_uh();
	return 0;
}
module_init(tasklet_module_init);

void __exit tasklet_module_exit(void)
{
	printk("tasklet: tasklet_module_exit()\n");
}
module_exit(tasklet_module_exit);
