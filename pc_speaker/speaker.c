#include <linux/module.h>
#include <asm/io.h>

#define FREQ 1193180

static void setup_freq(int nfreq)
{
	u32 div;
	div = FREQ / nfreq;
	outb(0xb6, 0x43);
	outb((u8) (div), 0x42);
	outb((u8) (div >> 8), 0x42);
}

static u8 port61;
static void enable(void)
{
	port61 = inb(0x61);
	printk("%s: port61 = %x\n", __FUNCTION__, port61);
	if (0x3 != (port61 & 3))
	{
		printk("%s: set port61 to %x\n", __FUNCTION__, port61|3);
		outb(port61 | 3, 0x61);
	}
	else
	{
		printk("%s: PC speaker already open.\n", __FUNCTION__);
	}
}
static void restore(void)
{
	outb(port61, 0x61);
}

static void play_sound(void)
{
	//setup_freq(800);
 	enable();
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
	restore();
}
module_exit(pcsp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("godspeed1989");

