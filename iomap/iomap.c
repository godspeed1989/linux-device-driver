/* 将映射到内核地址空间的I/O地址，通过ioctl(IOMAP_SET)，二次映射到新的地址空间。
 * 用户可通过read/write进行读写，也可以通过mmap进行映射。
 */
#include <asm/io.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include "iomap.h"

#define  IOMAP_DEV_MAJOR   42
#define  IOMAP_MAX_DEVS     6

#define  MSG(string, args...)  printk("iomap: " string, ##args);

struct Iomap* iomap_dev[IOMAP_MAX_DEVS];

ssize_t iomap_read(struct file *file, char __user *buf,
					size_t count, loff_t *offset)
{
	char *tmp;
	int minor = MINOR(file->f_dentry->d_inode->i_rdev);
	struct Iomap *idev = iomap_dev[minor];

	if(idev->base == 0) /* dev haven't been setup by 'IOMAP_SET'*/
		return -ENXIO;
	if(file->f_pos >= idev->size) /* beyond end */
		return 0;
	if(file->f_pos + count > idev->size) /*adjust beyond end*/
		count = idev->size - file->f_pos;
	tmp = (char*)kmalloc(count, GFP_KERNEL);
	if(tmp==NULL)
		return -ENOMEM;

	memcpy_fromio(tmp, idev->ptr+file->f_pos, count);
	if(copy_to_user(buf, tmp, count)) {
		kfree(tmp);
		return -EFAULT;
	}
	file->f_pos += count;
	
	kfree(tmp);
	return count;
}

ssize_t iomap_write(struct file *file, const char __user *buf,
					size_t count, loff_t *offset)
{
	char* tmp;
	int minor =  MINOR(file->f_dentry->d_inode->i_rdev);
	struct Iomap *idev = iomap_dev[minor];
	
	if(idev->base == 0) /* dev haven't been setup by 'IOMAP_SET'*/
		return -ENXIO;
	if(file->f_pos >= idev->size) /* beyond end */
		return 0;
	if(file->f_pos + count > idev->size) /*adjust beyond end*/
		count = idev->size - file->f_pos;
	tmp = (char*)kmalloc(count, GFP_KERNEL);
	if(tmp==NULL)
		return -ENOMEM;

	if(copy_from_user(tmp, buf, count)) {
		kfree(tmp);
		return -EFAULT;
	}
	memcpy_toio(idev->ptr+file->f_pos, tmp, count);
	file->f_pos += count;

	return count;
}

int iomap_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret;
	unsigned long size, pfn;
	int minor = MINOR(file->f_dentry->d_inode->i_rdev);
	struct Iomap *idev = iomap_dev[minor];
	MSG("in iomap_mmap\n");
	if(idev->base == 0) /* dev haven't set up */
		return -ENXIO;
	
	size = vma->vm_end - vma->vm_start;
	if(size % PAGE_SIZE)
		return -EINVAL;
	/* remap_pfn_range (vma, vaddr, pfn, size, prot) */
	# ifndef virt_to_pfn
	# define virt_to_pfn(kaddr) (__pa(kaddr) >> PAGE_SHIFT)
	# endif
	pfn = virt_to_pfn(idev->ptr);
	ret = remap_pfn_range(vma, vma->vm_start, pfn,
						  size, vma->vm_page_prot);
	if(ret)
		return -EAGAIN;
	MSG("iomap_mmap: mapped nr_pages = %lu \n", size/PAGE_SIZE);
	return 0;
}

long iomap_ioctl(struct file *file, unsigned int cmd,
									unsigned long arg)
{
	int minor = MINOR(file->f_dentry->d_inode->i_rdev);
	struct Iomap *idev = iomap_dev[minor];
	switch (cmd)
	{
		case IOMAP_SET: /* copy from user to setup */
		{
			if(idev->base)
				return -EBUSY;
			if(copy_from_user(idev, (struct Iomap*)arg, sizeof(struct Iomap)))
				return -EFAULT;
			if(idev->base % PAGE_SIZE || idev->size % PAGE_SIZE)
			{
				idev->base = 0;
				MSG("ioctl: IOMAP_SET: base and size must page align\n");
				return -EINVAL;
			}
			idev->ptr = ioremap(idev->base, idev->size); /*remap io addr*/
			if(idev->ptr == NULL)
			{
				MSG("ioctl: IOMAP_SET: ioremap failed\n");
				return -EFAULT;
			}
			MSG("ioctl: IOMAP_SET: minor %d\n", minor);
			MSG("ioctl: IOMAP_SET: from 0x%lx len=0x%lx\n",
										idev->base, idev->size);
			return 0;
		}
		case IOMAP_GET: /* copy to user for getting */
		{
			if(idev->base == 0)
				return -ENODEV;
			MSG("IOMAP_GET");
			if(copy_to_user((void*)arg, idev, sizeof(struct Iomap)))
				return -EFAULT;
			return 0;
		}
		case IOMAP_CLEAR:
		{
			long tmp;
			if(idev->base == 0)
				return -ENODEV;
			if(get_user(tmp, (long*)arg))
				return -EFAULT;
			MSG("IOMAP_CLEAR");
			memset_io(idev->ptr, tmp, idev->size);
			return 0;
		}
	}
	MSG("ioctl failed.\n");
	return -ENOTTY;
}

int iomap_open(struct inode *inode, struct file *file)
{
	if(MINOR(inode->i_rdev) >= IOMAP_MAX_DEVS)
		return -ENXIO;
	return 0;
}

int iomap_release(struct inode *inode, struct file *file)
{
	int minor = MINOR(inode->i_rdev);
	struct Iomap* idev = iomap_dev[minor];
	if(idev->base)
	{
		iounmap(idev->ptr);
		MSG("release: from 0x%lx len=0x%lx\n", idev->base, idev->size);
		idev->base = 0;
	}
	return 0;
}

struct file_operations iomap_fops = {
	.open = iomap_open,
	.release = iomap_release,
	.read = iomap_read,
	.write = iomap_write,
	.mmap = iomap_mmap,
	.unlocked_ioctl = iomap_ioctl,
};

int __init iomap_module_init(void)
{
	int i, ret;
	ret = register_chrdev(IOMAP_DEV_MAJOR, "remap", &iomap_fops);
	if(ret) {
		MSG("can't reg chr dev\n");
		return ret;
	}
	
	for (i=0 ; i<IOMAP_MAX_DEVS ; i++) {
		iomap_dev[i] = (struct Iomap*)kmalloc(sizeof(struct Iomap), GFP_KERNEL);
		iomap_dev[i]->base = 0;
	}
	MSG("init finished\n");
	return 0;
}
module_init(iomap_module_init);

void __exit iomap_module_exit(void)
{
	int i = 0;
	struct Iomap *tmp;
	
	for (tmp=iomap_dev[i] ; i<IOMAP_MAX_DEVS ; tmp = iomap_dev[++i])
	{
		if(tmp->base)
			iounmap(tmp->ptr);
		kfree(tmp);
	}
	unregister_chrdev(IOMAP_DEV_MAJOR, "remap");
}
module_exit(iomap_module_exit);
MODULE_LICENSE("GPL");
