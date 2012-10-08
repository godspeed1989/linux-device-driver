#include <linux/module.h>
#include <linux/sched.h>
#include "mysyscall.h"

unsigned long* get_sys_call_table(void)
{
	struct{
		unsigned short limit;
		unsigned int base;
	}__attribute__((packed)) idtr;
	
	struct{
		unsigned short offset_low;
		unsigned short segment_select;
		unsigned char reserved, flags;
		unsigned short offset_high;
	}__attribute__((packed)) *idt;

	unsigned long system_call = 0;
	char * call_hex="\xff\x14\x85";
	char * code_ptr = NULL;
	char * p = NULL;
	unsigned long sct = 0x0;
	int i = 0;
	
	asm volatile("sidt %0":"=m"(idtr));
	/*系统调用的中断向量号是0x80,偏移8*0x80个字节*/
	idt = (void *)(idtr.base + 8*0x80);
	system_call = (idt->offset_high<<16)|idt->offset_low;
	code_ptr = (char *)system_call;
	
	for(i=0 ; i<(100-2) ; i++)
	{//搜索call指令的机器码
		if(code_ptr[i] == call_hex[0]&&
		   code_ptr[i+1] == call_hex[1]&&
		   code_ptr[i+2] == call_hex[2])
		{
			p = &code_ptr[i] + 3;
			break;
		}
	}
	if(p)
	{
		sct = *(unsigned long *)p;
	}
	return (unsigned long *)sct;
}

unsigned int clear_and_get_cr0(void)
{
	static unsigned int cr0 = 0;
	unsigned int ret;

	asm volatile("movl %%cr0,%%eax":"=a"(cr0));
	ret = cr0;

	/*clear the 20th bit of CR0*/
	cr0 &= 0xfffeffff;
	asm volatile("movl %%eax, %%cr0"
		:
		:"a"(cr0));
	return ret;
}

void setback_cr0(unsigned int val)
{
	asm volatile("movl %%eax,%%cr0"
			:
			:"a"(val));
}

asmlinkage long sys_mycall(void)
{
	printk("Hello world!\n");
	printk("I am mycall.current->pid = %d,current->comm = %s\n",
	        current->pid,current->comm);
	return current->pid;
}

int orig_cr0;
unsigned long * sys_call_table;
static int (*something)(void);

static int __init init_addsyscall(void)
{
	sys_call_table =(unsigned long*)get_sys_call_table();

	something = (int(*)(void))(sys_call_table[G_NUM]);
	orig_cr0 = clear_and_get_cr0();
	sys_call_table[G_NUM] = (unsigned long)sys_mycall;
	setback_cr0(orig_cr0);
	
	printk("add_syscall init.\n");
	return 0;
}

void __exit exit_addsyscall(void)
{
	orig_cr0 = clear_and_get_cr0();
	sys_call_table[G_NUM] = (unsigned long)something;
	setback_cr0(orig_cr0);
	
	printk("add_syscall exit.\n");
}

module_init(init_addsyscall);
module_exit(exit_addsyscall);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Add System Call by Modify IDT");
