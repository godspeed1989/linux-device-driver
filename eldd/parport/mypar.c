#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/parport.h>

#define DEVICE_NAME    "led"

dev_t dev_number;
struct class *led_class;
struct cdev led_cdev;
struct pardevice *pdev;

int led_open(struct inode* inode, struct file *file)
{
	return 0;
}
ssize_t led_write(struct file *file, const char *buf,
				  size_t count, loff_t *ppos)
{
	char kbuf;
	if(copy_from_user(&kbuf, buf, 1)) return -EFAULT;
	
	parport_claim_or_block(pdev);
	parport_write_data(pdev->port, kbuf);
	parport_release(pdev);
	
	return count;
}

struct file_operations led_fops = 
{
	.owner	=	THIS_MODULE,
	.open	=	led_open,
	.write	=	led_write,
};

int led_preempt(void *handle)
{
	return 1;
}
void led_attach(struct parport *port)
{
	pdev = parport_register_device( port, DEVICE_NAME, led_preempt,
									NULL, NULL, 0, NULL );
	if(pdev == NULL)
		printk("led: attach: bad registe\n");
}
void led_detach(struct parport *port)
{
	printk("led: detach: device detachde\n");
}

struct parport_driver led_driver = 
{
	.name	=	"led",
	.attach	=	led_attach,
	.detach	=	led_detach,
};

int __init led_init(void)
{
	if(alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME) < 0)
	{
		printk("led: can't register device\n");
		return -1;
	}
	
	led_class = class_create(THIS_MODULE, DEVICE_NAME);
	if(IS_ERR(led_class))	printk("led: bad class create\n");
	
	cdev_init(&led_cdev, &led_fops); /* fops with cdev */
	led_cdev.owner = THIS_MODULE;
	
	if(cdev_add(&led_cdev, dev_number, 1))
	{
		printk("led: bad cdev add\n");
		return 1;
	}
	device_create(led_class, NULL, dev_number, NULL, DEVICE_NAME);
	
	if(parport_register_driver(&led_driver)) /* reg parport drv */
	{
		printk("led: bad register parport dev\n");
		return -EIO;
	}
	return 0;
}
module_init(led_init);

void __exit led_exit(void)
{
	unregister_chrdev_region(MAJOR(dev_number), 1);
	device_destroy( led_class, MKDEV(MAJOR(dev_number), 0) );
	class_destroy(led_class);
}
module_exit(led_exit);

MODULE_LICENSE("GPL");
