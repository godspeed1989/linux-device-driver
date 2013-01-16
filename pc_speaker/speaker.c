#include <linux/module.h>
#include <asm/io.h>

#define FREQ 1193180
static u8 port61;


static void setup_freq(int div)
{
	outb(0xb6, 0x43);
	outb((u8) (FREQ / div), 0x42);
	outb(((u8) (FREQ / div) >> 8), 0x42);
}

static void play_sound(void)
{
	setup_freq(1000);
 	//enable the PC speaker
	port61 = inb(0x61);
	if (port61 != (port61 | 3))
	{
		outb(port61 | 3, 0x61);
	}
	else
	{
		printk("%s: PC speaker already open.\n", __FUNCTION__);
	}
}

int __init pcsp_init(void)
{
	printk("%s\n", __FUNCTION__);
	play_sound();
	return 0;
}
module_init(pcsp_init);

void __exit pcsp_exit(void)
{
	printk("%s: restore the state of speaker.\n", __FUNCTION__);
	outb(port61, 0x61);
}
module_exit(pcsp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("godspeed1989");

