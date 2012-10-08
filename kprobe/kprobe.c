#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>

struct kprobe kp;

int handle_pre(struct kprobe *p, struct pt_regs *regs)
{
	printk(KERN_INFO"pt_regs: %p, pid: %d, jiffies: %ld\n",
	regs, current->tgid, jiffies);
	return 0;
}

static __init int init_kprobe_sample(void)
{
	/*kp.addr = (kprobe_opcode_t *)0x00;*/
	kp.symbol_name = "do_execve";
	kp.pre_handler = handle_pre;
	register_kprobe(&kp);
	return 0;
}
module_init(init_kprobe_sample);

static __exit void cleanup_kprobe_sample(void)
{
	unregister_kprobe(&kp);
}
module_exit(cleanup_kprobe_sample);

MODULE_LICENSE("GPL");
