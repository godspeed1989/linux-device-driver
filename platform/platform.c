#include <linux/module.h>
#include <linux/platform_device.h>

#define DEVICE_NAME "my_platform_dev"

#define  MSG(string, args...)  printk(DEVICE_NAME ": " string, ##args);

struct resource * reg_res;
unsigned long remap_size;
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
	MSG("platform_probe\n");
	// get register resource info
	reg_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(reg_res == NULL)
		return -ENODEV;
	remap_size = reg_res->end - reg_res->start + 1;
	// request region
	if (!request_mem_region(reg_res->start, remap_size, pdev->name))
		return -ENXIO;
	reg_base_virt = ioremap_nocache(reg_res->start, remap_size);
	if(reg_base_virt == NULL)
		return -EBUSY;
	return 0;
}

static int platform_remove(struct platform_device *pdev)
{
	/* Release mapped virtual address */
	iounmap(reg_base_virt);
	/* Release the region */
	release_mem_region(reg_res->start, remap_size);
	return 0;
}

/* device match table to match with device node in device tree */
static const struct of_device_id platform_of_match[] __devinitconst =
{
	{.compatible = DEVICE_NAME},
	{},
};
MODULE_DEVICE_TABLE(of, platform_of_match);

static struct platform_driver platform_drv =
{
	.probe = platform_probe,
	.remove = __devexit_p(platform_remove),
	.driver = 
	{
		.owner = THIS_MODULE,
		.name = DEVICE_NAME,
		.of_match_table = platform_of_match
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
	platform_device_unregister(&platform_dev);
	platform_driver_unregister(&platform_drv);
}
module_exit(platform_exit);
MODULE_LICENSE("GPL");

