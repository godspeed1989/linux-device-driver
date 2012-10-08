#include <linux/module.h>
#include <linux/interrupt.h>

int irq = 17;
module_param(irq,int,0);
char * interface = "eth1";
module_param(interface,charp,0);

irqreturn_t irq_hook(int irq, void* dev_id)
{
	static int cnt = 0;
	if(cnt < 10)
	{
		printk("rq_irq: capture\n");
		cnt++;
	}
	return IRQ_NONE;
}

int __init rq_irq_init(void)
{
	int ret;
	ret = request_irq(irq, &irq_hook, IRQF_SHARED, interface, &irq);
	if(ret)
	{
		printk("rq_irq: can't register irq\n");
		return -EIO;
	}
	printk("rq_irq: %s request on IRQ %d succeed\n", interface, irq);
	return 0;
}
module_init(rq_irq_init);

void __exit rq_irq_exit(void)
{
	free_irq(irq, &irq);
}
module_exit(rq_irq_exit);

MODULE_LICENSE("GPL");

/* sudo insmod rq_irq.ko interface=eth0 irq=9
 * cat /proc/interrupts
 */
