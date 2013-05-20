#include <linux/module.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/mman.h>
#include "share.h"

/*
	Remap a RAM or I/O region to kernel virtual address space and
	Export this region to user space through procfs.
 */
/*
	!!! critical !!!
	If you want to map a RAM region(beyond phyical memory high bounder),
	don't use request_region(...), open the macro MAP_RAM.
	A method to reserve RAM area is to use boot args 'mem=' or 'memmap='.
 */
#define DEVICE_NAME "my_resource"

#define  MSG(string, args...)  printk(DEVICE_NAME ": " string, ##args)

static void *rs_base_virt = NULL;
const static resource_size_t rs_start = 0xC0000000;
const static resource_size_t remap_size = (NR_PAGES << PAGE_SHIFT);

#define MAP_RAM

#ifndef MAP_RAM
static struct resource *res;
#endif

ssize_t proc_rs_read(struct file *file, char __user *buf,
                     size_t count, loff_t *offset)
{
	return 0;
}

static ssize_t proc_rs_write(struct file *file, const char __user * buf,
                             size_t count, loff_t * ppos)
{
	return 0;
}

static int proc_rs_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret;
	unsigned long size, pfn;
	size = vma->vm_end - vma->vm_start;
	if(size % PAGE_SIZE)
	{
		MSG("mmap size not page align error\n");
		return -EINVAL;
	}
	MSG("mmap try to mmap %ld pages\n", size>>PAGE_SHIFT);
#ifndef virt_to_pfn
#define virt_to_pfn(kaddr) (__pa(kaddr) >> PAGE_SHIFT)
#endif
	pfn = virt_to_pfn(rs_base_virt);
	/* Remap kernel virtual addr to user area */
	ret = remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot);
	if(ret != 0)
	{
		MSG("mmap remap_pfn_range error\n");
		return ret;
	}
	MSG("mmap %p to %lx\n", rs_base_virt, vma->vm_start);
	return 0;
}

static int proc_rs_open(struct inode *inode, struct file *file)
{
	MSG("/proc/" DEVICE_NAME " is opened\n");
	return 0;
}

static int proc_rs_release(struct inode *inode, struct file *file)
{
	MSG("/proc/" DEVICE_NAME " is closed\n");
	return 0;
}

static const struct file_operations proc_rs_operations =
{
	.open		= proc_rs_open,
	.release	= proc_rs_release,
	.read		= proc_rs_read,
	.write		= proc_rs_write,
	.mmap		= proc_rs_mmap,
};

int __init rs_init(void)
{
	struct proc_dir_entry *rs_proc_entry;
#ifndef MAP_RAM
	/* Request region */
	if(request_region(rs_start, remap_size , DEVICE_NAME) == NULL)
	{
		MSG("request region error\n");
		return -ENXIO;
	}
#endif
	/* Remap the region */
	rs_base_virt = ioremap(rs_start, remap_size);
	if(rs_base_virt == NULL)
	{
		MSG("remap region error\n");
#ifndef MAP_RAM
		release_mem_region(rs_start, remap_size);
#endif
		return -EBUSY;
	}
	MSG("remap phys=%x to virt=%p\n", rs_start, rs_base_virt);
	MSG("remap %d page(s) %d MB\n", remap_size>>PAGE_SHIFT, remap_size>>20);
	/* Create /proc entry */
	rs_proc_entry = proc_create(DEVICE_NAME, 0, NULL, &proc_rs_operations);
	if(rs_proc_entry == NULL)
	{
		MSG("create proc entry error\n");
		iounmap(rs_base_virt);
#ifndef MAP_RAM
		release_mem_region(rs_start, remap_size);
#endif		
		return -ENOMEM;
	}
	return 0;
}
module_init(rs_init);

void __exit rs_exit(void)
{
	/* Remove /proc entry */
	remove_proc_entry(DEVICE_NAME, NULL);
	/* Release mapped virtual address */
	iounmap(rs_base_virt);
#ifndef MAP_RAM
	/* Release requested region */
	release_mem_region(res->start, remap_size);
#endif
}
module_exit(rs_exit);

MODULE_LICENSE("Dual BSD/GPL");

