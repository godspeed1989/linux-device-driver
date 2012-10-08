#include "share.h"
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/vmalloc.h>
#include <linux/module.h>

#define DEV_MAJOR 250
static unsigned int major = DEV_MAJOR;

static char * p = NULL;
static int mmap_size = PAGE_SIZE*MAPPED_PAGES;

#define LINEAR 0
static int my_vmo_fault(struct vm_area_struct *vma, 
						struct vm_fault *vmf)
{
	unsigned long offset;
	struct page *page;
	
	printk("kmmap: in my_vmo_fault()\n");

	printk("kmmap: vma->vm_start = %lx\n", vma->vm_start);
	printk("kmmap: vmf->virtual_address = %lx\n", \
		(unsigned long)vmf->virtual_address);
	printk("kmmap: vmf->pgoff = %ld\n", vmf->pgoff);
#if LINEAR
	offset = (unsigned long)vmf->virtual_address - vma->vm_start;
	page = vmalloc_to_page(p+offset);
#else
	offset = vmf->pgoff<<PAGE_SHIFT;
	page = vmalloc_to_page(p+offset);
#endif
	if(!page)
		return VM_FAULT_SIGBUS;
	get_page(page);/* inc page's ref cnt */
	vmf->page = page;
	
	printk("kmmap: out my_vmo_fault()\n");
	return 0;
}
struct vm_operations_struct vm_ops = 
{
	.fault = my_vmo_fault, /* obselete nopage operation */
};

static int my_fops_mmap(struct file *f, struct vm_area_struct *vma)
{
	unsigned long vsize;
	
	printk("kmmap: in my_fops_mmap().\n");
	vsize = vma->vm_end - vma->vm_start;
	if( vsize > mmap_size) 
	{
		printk("kmmap: mmap size too large.\n");
		return -ENXIO;
	}
	
	vma->vm_ops = &vm_ops; /*set the vma operations*/
	vma->vm_flags |= VM_RESERVED;
	
	printk("kmmap: out my_fops_mmap().\n");
	return 0;
}

static int my_fops_open(struct inode *inode, struct file *f)
{
	printk("kmmap: my_fops_open()\n");
	return 0;
}
struct file_operations fops = {
	.open = my_fops_open,
	.mmap = my_fops_mmap,
};

static int my_module_init(void)
{
	int ret;
	unsigned long viraddr;
	
	ret = register_chrdev(major, "kmmap", &fops);
	if(ret)
	{
		printk("kmmap: reg char dev failed\n");
		return ret;
	}
	
	p = vmalloc(mmap_size);
	if(!p)
	{
		printk("kmmap: vmalloc fail\n");
		unregister_chrdev(major, "kmmap");
		return -ENOMEM;
	}
	
	viraddr = (unsigned long)p;
	while(viraddr <= ((unsigned long)p+PAGE_SIZE))
	{
		/*防止页面被swapout*/
		SetPageReserved(vmalloc_to_page((const void *)viraddr));
		viraddr += PAGE_SIZE;
	}

	strcpy(p, "hello abcd");
	strcpy(p+PAGE_SIZE, "hello defg");
	return ret;
}
module_init(my_module_init);

static void my_module_exit(void)
{
	unsigned long viraddr;
	viraddr = (unsigned long)p;
	while(viraddr <= ((unsigned long)p+PAGE_SIZE))
	{
		/*允许页面被swapout*/
		ClearPageReserved(vmalloc_to_page((const void *)viraddr));
		viraddr += PAGE_SIZE;
	}
	
	vfree(p);
	unregister_chrdev(major, "kmmap");
}
module_exit(my_module_exit);
MODULE_LICENSE("GPL");
