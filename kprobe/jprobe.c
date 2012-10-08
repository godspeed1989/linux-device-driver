#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>

struct jprobe jp;

int jp_do_execve(char* filename, char __user*__user* argv, 
			char __user*__user* envp, struct pt_regs* regs)
{
	int cnt = 0;
		printk("---filename: %s\n", filename);
	for(; *argv!=NULL; argv++, cnt++)
	printk("argv[%d]: %s\n", cnt, *argv);
	jprobe_return();
	return 0;/*不会被执行，为避免编译器报错而写*/
}

static __init int init_jprobe_sample(void)
{
	/*指定被侦测的函数名和侦测器函数*/
	jp.kp.symbol_name = "do_execve";
	jp.entry = JPROBE_ENTRY(jp_do_execve);
	register_jprobe(&jp);
	return 0;
}
module_init(init_jprobe_sample);

static __exit void cleanup_jprobe_sample(void)
{
	unregister_jprobe(&jp);
}
module_exit(cleanup_jprobe_sample);

MODULE_LICENSE("GPL");

