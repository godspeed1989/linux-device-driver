#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include "mysyscall.h"
/* get addr by cat /proc/kallsyms | grep sys_call_table */
#define sys_call_table_addr 0xc1513160

unsigned int clear_and_return_cr0(void);
void set_back_cr0(unsigned int val);

int orig_cr0;
unsigned long *sys_call_table = 0;
static int (*func_point)(void);

unsigned int clear_and_return_cr0(void)
{
	unsigned int cr0;
	unsigned int ret;

	asm volatile("movl %%cr0, %%eax":"=a"(cr0));
	ret = cr0;
	cr0 &= 0xfffeffff;
	asm volatile("movl %%eax, %%cr0"::"a"(cr0));
	return ret;
}

void set_back_cr0(unsigned int val)
{
	asm volatile("movl %%eax, %%cr0"::"a"(val));
}

asmlinkage long sys_mycall(void)
{
	printk("mycall1. current->pid is %d, current->comm is %s\n", \
			current->pid, current->comm);
	return current->pid;
}

static int __init init_addsyscall(void)
{
	printk("start...\n");
	sys_call_table = (unsigned long *)sys_call_table_addr;

	func_point = (int(*)(void))(sys_call_table[G_NUM]);
	orig_cr0 = clear_and_return_cr0();
	sys_call_table[G_NUM] = (unsigned long)&sys_mycall;
	set_back_cr0(orig_cr0);
	return 0;
}

static void __exit exit_addsyscall(void)
{
	orig_cr0 = clear_and_return_cr0();
	sys_call_table[G_NUM] = (unsigned long)func_point;
	set_back_cr0(orig_cr0);
}

module_init(init_addsyscall);
module_exit(exit_addsyscall);
MODULE_LICENSE("GPL");
