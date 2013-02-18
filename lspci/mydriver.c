#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#define PCIIO_SIZE	0x0500*6	/*����I/O�豸��������5�����ߺţ�32���豸�ţ�8�����ܺţ�����6���ֶΣ����ļ��Ĵ�С */
#define PCIIO_MAJOR 250			/*Ԥ���pciio�����豸��*/

static int pciio_major = PCIIO_MAJOR;

/*pciio�豸�ṹ��*/
struct pciio_dev
{
	struct cdev cdev; /*cdev�ṹ��*/
	/*�����й�Ӧ�̡��豸���롢���ߺš��豸�š����ܺš���ַ�Ĵ���������*/
	u32 pciio[PCIIO_SIZE];
};

struct pciio_dev *pciio_devp; /*�豸�ṹ��ָ��*/

/*�ļ��򿪺���*/
int pciio_open(struct inode *inode, struct file *filp)
{
	/*���豸�ṹ��ָ�븳ֵ���ļ�˽������ָ��*/
	filp->private_data = pciio_devp;
	return 0;
}
/*�ļ��ͷź���*/
int pciio_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/*������*/
static ssize_t pciio_read(struct file *filp, char __user *buf, size_t size,
	loff_t *ppos)
{
	unsigned long p =  *ppos;/*p:ƫ����*/
	unsigned int count = size;/*count:��ȡ�Ĵ�С*/
	int ret = 0;
	struct pciio_dev *dev = filp->private_data; /*����豸�ṹ��ָ��*/

	/*�����ͻ�ȡ��Ч��д����*/
	if (p >= PCIIO_SIZE)
		return 0;/*ƫ�ƴ����ļ����ȣ�ֹͣ���ļ�*/
	if (count > PCIIO_SIZE - p)
		count = PCIIO_SIZE - p;/*ʣ�ಿ�ֲ�������ȡ�Ĵ�С*/
	
	if(copy_to_user(buf, (void *)(dev->pciio + p), count))
	{
		ret =  - EFAULT;
	} 
	else
	{
		*ppos += count;	/*�ı�ƫ����*/
		ret = count;	/*���ض�ȡ�ֽڵĴ�С*/
		printk(KERN_INFO "read %u bytes(s) from %lu\n", count, p);
	}
	return ret;
}

/*�ļ������ṹ��*/
static const struct file_operations pciio_fops =
{
	.owner = THIS_MODULE,
	.read = pciio_read,
	.open = pciio_open,
	.release = pciio_release,
};

/*��ʼ����ע��cdev*/
static void pciio_setup_cdev(struct pciio_dev *dev, int index)
{
	int err, devno = MKDEV(pciio_major, index);

	cdev_init(&dev->cdev, &pciio_fops);		/*��ʼ���豸*/
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);	/*����豸*/
	if (err)
		printk(KERN_NOTICE "Error");
}

/*�豸����ģ����غ���*/
int pciio_init(void)
{
	int result;
	dev_t devno = MKDEV(pciio_major, 0);/*�������豸�źϳ��豸���*/
	u32 i,busno,deviceno,funcno,regval,retval,venderID,devID;
	u32 basereg;

	/* �����豸��*/
	if (pciio_major)/*������豸��δ��ռ��*/
		result = register_chrdev_region(devno, 1, "pciio");/*�ֹ��������豸��*/
	else
	{
		result = alloc_chrdev_region(&devno, 0, 1, "pciio");/*��̬�����豸��*/
		pciio_major = MAJOR(devno);/*��ȡ���豸��*/
	}
	if (result < 0)
		return result;

	/* ��̬�����豸�ṹ����ڴ�*/
	pciio_devp = kmalloc(sizeof(struct pciio_dev), GFP_KERNEL);
	if (!pciio_devp)
	{
		result = -ENOMEM;
		goto fail_malloc;
	}

	memset(pciio_devp, 0, sizeof(struct pciio_dev));	/*��ʼ���豸�ṹ��Ϊ0*/
	pciio_setup_cdev(pciio_devp, 0);	/*����cdev��ʼ����ע�ắ��*/

	/*��ʼ���豸�ļ�����PCI���õ�ַ�Ĵ���д�����ݣ�
	  ��PCI�������ݼĴ����������ݣ������ݱ������豸�ṹ���int��������*/
	i = 0;
	for (busno=0;busno<5;busno++)	/*ɨ�����ߺ�*/
	{
   		for (deviceno=0;deviceno<32;deviceno++)/*ɨ���豸��*/
   		{
			for (funcno=0;funcno<8;funcno++)/*ɨ�蹦�ܺ�*/
			{
	    		regval=0x80000000+(busno<<16)+(deviceno<<11)+(funcno<<8);/*����Ŀ���ַ����0�żĴ�����ʼ*/
	    		outl(0x0CF8,regval);	/*��Ŀ���ַд���ַ�Ĵ���*/
	    		retval=inl(0x0CFC);		/*�����ݼĴ����ж�����*/
	    		if(retval!=0xffffffff)	/*������ڸ�PCI����*/
	    		{	
					venderID = retval&0xffff;	/*��ȡ��Ӧ�̴���*/
					devID = (retval>>16)&0xffff;/*��ȡ�豸����*/
					regval += 0x10;				/*����10H��ַ�Ĵ���*/
					outl(0x0CF8, regval);
					retval=inl(0x0CFC);			/*��ȡ��һ��32λ����ַ�Ĵ�������*/
					if ((retval&0x1) == 0)		/*����Ǵ洢������ַ*/
		    			basereg=(retval)&0xfffffff0;
					else						/*�����I/O����ַ*/
		   				basereg=(retval)&0xfffffffc;
		   			/*������������д���豸�ṹ���������*/
		   			*(pciio_devp->pciio+i*6) = venderID;
					*(pciio_devp->pciio+i*6+1) = devID;
					*(pciio_devp->pciio+i*6+2) = busno;
					*(pciio_devp->pciio+i*6+3) = deviceno;
					*(pciio_devp->pciio+i*6+4) = funcno;
					*(pciio_devp->pciio+i*6+5) = basereg;
					i++;
					if (funcno==0)
					{	/*����ǵ������豸���Ͳ��ٰ����ܺ�ɨ��*/
		    			regval=(regval&0xfffffff0)+0x0c;
		    			outl(0x0CF8,regval);
		    			retval=inl(0x0CFC);
		    			retval=retval>>16;
		    			if ((retval&0x80)==0)/*ͷ�����ͼĴ�����һλָʾ���Ƿ��ǵ������豸*/
							funcno=8;
					}
	    		}
			}//for
    	}//for
	}//for
	return 0;
fail_malloc:
	unregister_chrdev_region(devno, 1);
	return result;
}

/*ģ��ж�غ���*/
void pciio_exit(void)
{
	cdev_del(&pciio_devp->cdev);   /*ע��cdev*/
	unregister_chrdev_region(MKDEV(pciio_major, 0), 1); /*�ͷ��豸��*/
}

MODULE_LICENSE("Dual BSD/GPL");
module_param(pciio_major, int, S_IRUGO);
module_init(pciio_init);
module_exit(pciio_exit);

