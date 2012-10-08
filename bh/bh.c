#include <linux/module.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/uaccess.h> /*copy from/to use*/

#define MAJOR_NUM 250

unsigned int cnt;  /*char dev ref count*/
spinlock_t cnt_spin; /*ref count spin lock*/
unsigned int globalvar;
struct semaphore sem; /*global var's semaphore*/
wait_queue_head_t waitq; /*read/write operation wait queue*/
int flag;

int globalvar_open(struct inode *node, struct file *f)
{
	spin_lock(&cnt_spin);
	if(cnt > 2)
	{
		spin_unlock(&cnt_spin);
		return -EBUSY;
	}
	cnt++;
	spin_unlock(&cnt_spin);
	return 0;
}
int globalvar_release(struct inode* node, struct file *f)
{
	cnt--;
	return 0;
}

ssize_t globalvar_read(struct file* f, char* buf, size_t len, loff_t *off)
{
	if(wait_event_interruptible(waitq, flag!=0))
	{
		return -ERESTARTSYS;
	}
	flag = 0;
	/* down() 不能被信号打断，会导致睡眠
	 * down_interruptible() 能被信号打断
	 * down_trylock() 尝试获得，立即返回结果
	 */
	if(down_interruptible(&sem))
	{
		return -ERESTARTSYS;
	}
	if(copy_to_user(buf,&globalvar,sizeof(int)))
	{
		up(&sem);
		return -EFAULT;
	}
	up(&sem);
	return sizeof(int);
}

ssize_t globalvar_write(struct file *f, const char *buf,
						size_t len, loff_t *off)
{
	if(down_interruptible(&sem))
	{
		return -ERESTARTSYS;
	}
	if(copy_from_user(&globalvar,buf,sizeof(int)))
	{
		up(&sem);
		return -EFAULT;
	}
	up(&sem);
	flag = 1;
	wake_up_interruptible(&waitq);
	return sizeof(int);
}

struct file_operations globalvar_fops = {
	.open = globalvar_open,
	.release = globalvar_release,
	.read = globalvar_read,
	.write = globalvar_write,
};

int __init globalvar_init(void)
{
	int ret;
	cnt = 0;
	ret = register_chrdev(MAJOR_NUM, "mylock", &globalvar_fops);
	if(ret<0)
	{
		printk("locktest: char dev reg failed.\n");
		return ret;
	}
	spin_lock_init(&cnt_spin);
	sema_init(&sem, 1);
	init_waitqueue_head(&waitq);
	flag = 0;
	return 0;
}
module_init(globalvar_init);

void __exit globalvar_exit(void)
{
	unregister_chrdev(MAJOR_NUM, "mylock");
	printk("locktest: exit...\n");
}
module_exit(globalvar_exit);
MODULE_LICENSE("GPL");
