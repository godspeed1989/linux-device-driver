#include <linux/module.h>
#include <linux/slab.h>
// map a RAM or I/O region to kernel virtual address space.
/*
	!!! critical
	1. if you want to map a RAM region(beyond phyical memory high bounder),
	   don't use request_region(...).
	2. remap_pfn_range()/io_remap_pfn_range is better than ioremap() when you want to map RAM.
 */
#define DEVICE_NAME "my_resource"

#define  MSG(string, args...)  printk(DEVICE_NAME ": " string, ##args)

void *reg_base_virt = NULL;
const resource_size_t rs_start = 0xD0000000;
const resource_size_t remap_size = 0x00800000;
#define MAP_RAM

#ifndef MAP_RAM
static struct resource *res;
#endif

int __init rs_init(void)
{
#ifndef MAP_RAM
	/* Request region */
	res = request_region(rs_start, remap_size , DEVICE_NAME);
	if (res == NULL)
	{
		MSG("request region error\n");
		return -ENXIO;
	}
#endif
	/* Remap the region */
	reg_base_virt = ioremap(rs_start, remap_size);
	if(reg_base_virt == NULL)
	{
		MSG("remap region error\n");
		release_mem_region(rs_start, remap_size);
		return -EBUSY;
	}
	MSG("remap phys=%x to virt=%p\n", rs_start, reg_base_virt);
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
	/* Release mapped virtual address */
	iounmap(reg_base_virt);
#ifndef MAP_RAM
	if(res != NULL)
	{
		/* Release the region */
		release_mem_region(res->start, remap_size);
		/* free */
		kfree(res);
	}
#endif
}
module_exit(rs_exit);
MODULE_LICENSE("GPL");
