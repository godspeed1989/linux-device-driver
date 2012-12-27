#include <linux/module.h>
#include <asm/io.h>

u8 tmp;

int __init pcsp_init(void)
{
	printk("%s\n", __FUNCTION__);

 	outb(0x43, 0xb6);
 	outb(0x42, (u8) (1193180 / 1000) );
 	outb(0x42, (u8) ((1193180 / 1000) >> 8));
 	
	tmp = inb(0x61);
	if (tmp != (tmp | 3))
	{
		outb(0x61, tmp | 3);
	}
	return 0;
}
module_init(pcsp_init);

void __exit pcsp_exit(void)
{
	outb(0x61, tmp);
}
module_exit(pcsp_exit);
