#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/completion.h>

typedef  void (*FUNCTION_T)(void);
#define  EXT_DAT_TYPE  long
struct dev_drv
{
	FUNCTION_T start_work;
	FUNCTION_T do_work;
	void *context;
	unsigned int nr_ext;
	EXT_DAT_TYPE ext_dat[0];
};

struct dev_drv *drv;
struct completion done;
DECLARE_WAITQUEUE(queue, NULL);

void start_work(void)
{
	int retn;
	unsigned long expire = msecs_to_jiffies(100);
	
	init_completion(&done);
	drv->context = &done;

	drv->do_work();

	retn = wait_for_completion_timeout(&done, expire);
	printk("completion: expires = %lu\n", expire);
	if(retn==0)
	{
		printk("completion: work failed\n");
	}
	else
	{
		printk("completion: work succeed: %d advanced\n", retn);
	}
}

void do_work(void)
{
	long i;
	printk("completion: start to work\n");
	for(i=0; i<ULONG_MAX; i++);
	complete((struct completion*)drv->context);
	printk("completion: finish to work\n");
}

int __init completion_mod_init(void)
{
	int nr_ext = 5;
	drv = kmalloc(sizeof(struct dev_drv) + 
				  nr_ext*sizeof(EXT_DAT_TYPE), GFP_KERNEL);
	drv->nr_ext = nr_ext;
	drv->ext_dat[0] = 0;
	drv->ext_dat[1] = 1;
	drv->start_work = start_work;
	drv->do_work = do_work;
	
	drv->start_work();
	
	return 0;
}
module_init(completion_mod_init);

void __exit completion_mod_exit(void)
{
	kfree(drv);
}
module_exit(completion_mod_exit);
MODULE_LICENSE("GPL");
