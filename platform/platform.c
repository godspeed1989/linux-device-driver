#include <linux/module.h>
#include <linux/platform_device.h>

#define DEVICE_NAME "my_platform_dev"

#define  MSG(string, args...)  printk(DEVICE_NAME ": " string, ##args);

void *reg_base_virt;

static struct resource platform_rs[] = 
{
	{
		.start = 0x40030000,
		.end   = 0x40030000 + 4*4,
		.flags = IORESOURCE_MEM,
	}
};

static struct platform_device platform_dev =
{
	.name = DEVICE_NAME,
	.id = 0,
	.dev = {
		.release = NULL,
	},
	.resource = platform_rs,
	.num_resources = ARRAY_SIZE(platform_rs),
};

static int platform_probe(struct platform_device *pdev)
{
	struct resource * reg_res;
	MSG("platform_probe\n");
	// get register resource info
	reg_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(reg_res == NULL)
		return -ENODEV;
	reg_base_virt = ioremap_nocache(reg_res->start, reg_res->end-reg_res->start);
	if(reg_base_virt == NULL)
		return -EBUSY;
	return 0;
}

static struct platform_driver platform_drv =
{
	.probe = platform_probe,
	.driver = 
	{
		.owner = THIS_MODULE,
		.name = DEVICE_NAME,
	},
};

int __init platform_init(void)
{
	int ret;
	ret = platform_device_register(&platform_dev);
	if(ret)
	{
		return ret;
	}
	ret = platform_driver_register(&platform_drv);
	if(ret)
	{
		return ret;
	}
	return 0;
}
module_init(platform_init);

void __exit platform_exit(void)
{
}
module_exit(platform_exit);
MODULE_LICENSE("GPL");

