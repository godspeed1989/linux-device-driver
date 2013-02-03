#include <linux/module.h>
#include <linux/i8253.h>
#include <asm/io.h>

#define FREQ 1193180

static void setup_freq(int nfreq)
{
	u16 div;
	unsigned long flags;
	raw_spin_lock_irqsave(&i8253_lock, flags);
	printk("%s:\n", __FUNCTION__);
	div = FREQ / nfreq;
	outb_p(0xB6, 0x43);
	outb_p(div & 0xff, 0x42);
	outb_p((div >> 8) & 0xff, 0x42);
	raw_spin_unlock_irqrestore(&i8253_lock, flags);
}

static u8 port61;
static void enable(void)
{
	port61 = inb_p(0x61);
	printk("%s: port61 = %x\n", __FUNCTION__, port61);
	if (0x3 != (port61 & 3))
	{
		printk("%s: set port61 to %x\n", __FUNCTION__, port61|3);
		outb_p(inb_p(0x61) | 3, 0x61);
	}
	else
	{
		printk("%s: PC speaker already open.\n", __FUNCTION__);
	}
}
static void restore(void)
{
	outb_p(port61, 0x61);
}

static void play_sound(void)
{
	enable();
	setup_freq(1000);
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

