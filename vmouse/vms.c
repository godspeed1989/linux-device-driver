#include <linux/fs.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#define  MSG(string, args...)  printk("vmouse: " string, ##args);

struct input_dev *vms_input_dev;
struct platform_device *vms_plat_dev;

ssize_t vms_store(struct device *dev, struct device_attribute *attr,
				  const char *buffer, size_t count)
{
	int x, y;
	sscanf(buffer, "%d%d", &x, &y);
	
	input_report_rel(vms_input_dev, REL_X, x);
	input_report_rel(vms_input_dev, REL_Y, y);
	input_sync(vms_input_dev);
	
	return 0;
}
/* DEVICE_ATTR(_name, _mode, _show, _store), for sysfs */
DEVICE_ATTR(coordinates, 0644, NULL, vms_store);

struct attribute *vms_attrs[] = {
	&dev_attr_coordinates.attr,
	NULL
};

struct attribute_group vms_attr_group = {
	.attrs = vms_attrs,
};

int __init vms_init(void)
{
	vms_plat_dev = platform_device_register_simple("vms", -1, NULL, 0);
	if(IS_ERR(vms_plat_dev))
	{
		PTR_ERR(vms_plat_dev);
		MSG("init: reg platform driver err\n");
	}
	/* create sysfs node to read simulated coordinates */
	sysfs_create_group(&vms_plat_dev->dev.kobj, &vms_attr_group);
	
	vms_input_dev = input_allocate_device();
	if(!vms_input_dev)
	{
		MSG("init: alloc input dev err\n");
	}
	set_bit(EV_REL, vms_input_dev->evbit); /*capture relative axes event*/
	set_bit(REL_X, vms_input_dev->relbit); /*produce axe 'X' movement*/
	set_bit(REL_Y, vms_input_dev->relbit); /*produce axe 'Y' movement*/
	
	input_register_device(vms_input_dev);
	
	return 0;
}
module_init(vms_init);

void __exit vms_exit(void)
{
	input_unregister_device(vms_input_dev);
	sysfs_remove_group(&vms_plat_dev->dev.kobj, &vms_attr_group);
	platform_device_unregister(vms_plat_dev);
}
module_exit(vms_exit);
MODULE_LICENSE("GPL");
