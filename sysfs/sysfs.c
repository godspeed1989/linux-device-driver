#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/proc_fs.h>
#include <linux/sysfs.h>
#include <linux/device.h>

struct class *sysfs_class;

struct attribute hello_attr;
struct attribute goodbye_attr;
struct attribute *sysfs_attrs[] =
{
	&hello_attr,
	&goodbye_attr,
	NULL
};

struct sysfs_ops sysfs_ops = {
	
};

struct kobject kobject;
struct kobj_type kobject_type = 
{
	.sysfs_ops = &sysfs_ops,
	.default_attrs = sysfs_attrs,
};

int __init sysfs_mod_init(void)
{
	sysfs_class = class_create(THIS_MODULE, "yanlin");
	if(IS_ERR(sysfs_class))
	{
		printk("sysfs: class create err\n");
		return -ENOMEM;
	}
	
	kobject_init(&kobject, &kobject_type);
	if(kobject_set_name(&kobject, "yanlin") < 0)
	{
		return -ENOMEM;
	}
	kobject_add(&kobject, NULL, "yanlin");
	
	return 0;
}
module_init(sysfs_mod_init);

void __exit sysfs_mod_exit(void)
{
	kobject_del(&kobject);
}
module_exit(sysfs_mod_exit);
MODULE_LICENSE("GPL");
