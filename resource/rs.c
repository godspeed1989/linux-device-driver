#include <linux/module.h>
#include <linux/slab.h>

#define DEVICE_NAME "my_resource"

#define  MSG(string, args...)  printk(DEVICE_NAME ": " string, ##args)

void *reg_base_virt = NULL;

const resource_size_t rs_start = 0xD0000000;
const resource_size_t remap_size = 0x00800000;
static struct resource *res;

int __init rs_init(void)
{
	/* Request region */
	res = request_mem_region(rs_start, remap_size , DEVICE_NAME);
	if (res == NULL)
	{
		MSG("request region error\n");
		return -ENXIO;
	}
	/* Remap the region */
	reg_base_virt = ioremap(res->start, remap_size);
	if(reg_base_virt == NULL)
	{
		MSG("remap region error\n");
		release_mem_region(res->start, remap_size);
		return -EBUSY;
	}
	MSG("remap phys=%x to virt=%p\n", res->start, reg_base_virt);
	/* test write */
	strcpy(reg_base_virt, DEVICE_NAME);
	if(strcmp(reg_base_virt, DEVICE_NAME))
		MSG("test write failed\n");
	else
		MSG("test write succeed\n");
	return 0;
}
module_init(rs_init);

void __exit rs_exit(void)
{
	if(res != NULL)
	{
		/* Release mapped virtual address */
		iounmap(reg_base_virt);
		/* Release the region */
		release_mem_region(res->start, remap_size);
		/* free */
		kfree(res);
	}
}
module_exit(rs_exit);
MODULE_LICENSE("GPL");
