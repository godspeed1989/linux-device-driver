#include <linux/module.h>

#define DEVICE_NAME "my_resource"

#define  MSG(string, args...)  printk(DEVICE_NAME ": " string, ##args);

unsigned long remap_size = 0;
void *reg_base_virt = NULL;

static struct resource rs =
{
	.start = 0xdafde000,
	.end   = 0xdafde000 + 0x0000ff,
	.flags = IORESOURCE_MEM,
};

int __init rs_init(void)
{
	/* Request region */
	remap_size = rs.end - rs.start + 1;
	if (!request_mem_region(rs.start, remap_size , DEVICE_NAME))
		return -ENXIO;
	/* Remap the region */
	reg_base_virt = ioremap_nocache(rs.start, remap_size);
	if(reg_base_virt == NULL)
	{
		release_mem_region(rs.start, remap_size);
		return -EBUSY;
	}
	return 0;
}
module_init(rs_init);

void __exit rs_exit(void)
{
	/* Release mapped virtual address */
	if(reg_base_virt != NULL)
	{
		iounmap(reg_base_virt);
		/* Release the region */
		release_mem_region(rs.start, remap_size);
	}
}
module_exit(rs_exit);
MODULE_LICENSE("GPL");

