#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include "cmos.h"

unsigned char addr_ports[NUM_CMOS_BANKS] = {CMOS_BANK0_INDEX_PORT,
											CMOS_BANK1_INDEX_PORT,};
unsigned char data_ports[NUM_CMOS_BANKS] = {CMOS_BANK0_DATA_PORT,
											CMOS_BANK1_DATA_PORT,};

struct cmos_dev
{
	struct cdev cdev;
	int bank_number;
	unsigned short current_pointer;
	unsigned int size;
	char name[10];
	/* ... */
};

dev_t cmos_dev_number;
struct class *cmos_class;
struct cmos_dev *cmos_devs[NUM_CMOS_BANKS];

extern unsigned int port_data_in(unsigned char offset, int bank);
extern void port_data_out(unsigned char offset, unsigned char data, int bank);

ssize_t cmos_read(struct file* file, char *buf,
				  size_t count, loff_t *ppos)
{
	struct cmos_dev *cmos_devp = file->private_data;
	char data[CMOS_BANK_SIZE];
	unsigned char mask;
	int xferred = 0, i = 0, zero_out;
	int start_byte = cmos_devp->current_pointer/8;
	int start_bit  = cmos_devp->current_pointer%8;
	
	if(cmos_devp->current_pointer >= cmos_devp->size)
		return 0; /*EOF*/
	if(cmos_devp->current_pointer + count >= cmos_devp->size)
		count = cmos_devp->size - cmos_devp->current_pointer;
	
	while(xferred < count)
	{
		data[i] = port_data_in(start_byte, cmos_devp->bank_number);
		data[i] >>= start_bit;
		xferred += 8 - start_bit;
		if(start_bit && (count+start_bit>8))
		{
			data[i] |= (port_data_in(start_byte+1, 
						cmos_devp->bank_number) << (8-start_bit));
			xferred += start_bit;
		}
		start_byte++;
		i++;
	}
	if(xferred > count)
	{
		zero_out = xferred - count;
		mask = 1 << (8 - zero_out + 1);
		mask--;
		data[i-1] &= ~mask;
		xferred = count;
	}
	
	if(!xferred) return -EIO;
	if(copy_to_user(buf, (void*)data, (xferred/8+1)) != 0)
		return -EIO;
	
	cmos_devp->current_pointer += xferred;
	return xferred;
}

ssize_t cmos_write(struct file *file, const char* buf,
				   size_t count, loff_t *ppos)
{
	return 0;
}

loff_t cmos_llseek(struct file *file, loff_t offset, int orig)
{
	struct cmos_dev *cmos_devp = file->private_data;
	switch (orig)
	{
		case SEEK_SET:
			if(offset >= cmos_devp->size) return -EINVAL;
			cmos_devp->current_pointer = offset;
			break;
		case SEEK_CUR:
			if(cmos_devp->current_pointer+offset >= cmos_devp->size)
				return -EINVAL;
		case SEEK_END:
			return -EINVAL;
		default:
			return -EINVAL;
	}
	return cmos_devp->current_pointer;
}

#define CMOS_ADJUST_CHECKSUM 1
#define CMOS_VERIFY_CHECKSUM 2
#define CMOS_BANK1_CRC_OFFSET 0x1E
long cmos_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned short crc = 0;
	unsigned char buf;
	switch (cmd)
	{
		case CMOS_ADJUST_CHECKSUM:
			crc = 0xABCD;/* adjust_cmos_crc */
			port_data_out(CMOS_BANK1_CRC_OFFSET,
						(unsigned char)(crc&0xFF), 1);
			port_data_out(CMOS_BANK1_CRC_OFFSET,
						(unsigned char)(crc >> 8), 1);
		case CMOS_VERIFY_CHECKSUM:
			crc = 0xABCD;/* adjust_cmos_crc */
			buf = port_data_in(CMOS_BANK1_CRC_OFFSET, 1);
			if(buf != (unsigned char)(crc&0xFF)) return -EINVAL;
			buf = port_data_in(CMOS_BANK1_CRC_OFFSET+1, 1);
			if(buf != (unsigned char)(crc >> 8)) return -EINVAL;
			break;
		default:
			return -EIO;
	}
	return 0;
}

int cmos_open(struct inode *inode, struct file *file)
{
	struct cmos_dev *cmos_devp;
	cmos_devp = container_of(inode->i_cdev, struct cmos_dev, cdev);
	cmos_devp->size = CMOS_BANK_SIZE;
	cmos_devp->current_pointer = 0;
	
	file->private_data = cmos_devp;
	return 0;
}

int cmos_release(struct inode *inode, struct file *file)
{
	struct cmos_dev *cmos_devp = file->private_data;
	cmos_devp->current_pointer = 0;
	return 0;
}

struct file_operations cmos_fops =
{
	.owner		=	THIS_MODULE,
	.open		=	cmos_open,
	.release	=	cmos_release,
	.read		=	cmos_read,
	.write		=	cmos_write,
	.llseek		=	cmos_llseek,
	.unlocked_ioctl = cmos_ioctl,
};

int __init cmos_init(void)
{
	int i, r;
	r = alloc_chrdev_region(&cmos_dev_number, 0, \
							NUM_CMOS_BANKS, DEVICE_NAME);
	if(r<0) {
		MSG("can't alloc chrdev region\n"); return -ENOMEM;
	}
	/* populate sysfs entry */
	cmos_class = class_create(THIS_MODULE, DEVICE_NAME);
	
	for (i=0 ; i<NUM_CMOS_BANKS ; i++)
	{
		cmos_devs[i] = kmalloc(sizeof(struct cmos_dev), GFP_KERNEL);
		if(!cmos_devs[i]) {
			MSG("err alloc cmos_dev mem\n"); return -ENOMEM;
		}
		sprintf(cmos_devs[i]->name, "cmos%d", i);
		/* request I/O region */
		if( !request_region(addr_ports[i], 2, cmos_devs[i]->name) ) 
		{
			MSG("I/O port 0x%x busy\n", addr_ports[i]);
			kfree(cmos_devs[i]);
			return -EIO;
		}
		
		cmos_devs[i]->bank_number = i;
		cdev_init(&cmos_devs[i]->cdev, &cmos_fops);
		cmos_devs[i]->cdev.owner = THIS_MODULE;
		
		r = cdev_add(&cmos_devs[i]->cdev, (cmos_dev_number+i), 1);
		if(r) {
			MSG("cdev_add err\n"); return -EBUSY;
		}
		/* send uevents to udev to create /dev/cmos%d nodes */
		device_create(cmos_class, 0, cmos_dev_number+i, 0, "cmos%d", i);
	}
	
	return 0;
}
module_init(cmos_init);
void __exit cmos_exit(void)
{
	int i;
	unregister_chrdev_region(cmos_dev_number, NUM_CMOS_BANKS);
	
	for (i=0 ; i<NUM_CMOS_BANKS ; i++)
	{
		device_destroy( cmos_class, MKDEV(MAJOR(cmos_dev_number),i) );
		release_region(addr_ports[i], 2);
		cdev_del(&cmos_devs[i]->cdev);
		kfree(cmos_devs[i]);
	}
	class_destroy(cmos_class);
}
module_exit(cmos_exit);
MODULE_LICENSE("GPL");
