#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#define PCIIO_SIZE	0x0500*6	/*所有I/O设备最大计数（5个总线号，32个设备号，8个功能号）乘以6个字段，即文件的大小 */
#define PCIIO_MAJOR 250			/*预设的pciio的主设备号*/

static int pciio_major = PCIIO_MAJOR;

/*pciio设备结构体*/
struct pciio_dev
{
	struct cdev cdev; /*cdev结构体*/
	/*保存有供应商、设备代码、总线号、设备号、功能号、基址寄存器的数组*/
	u32 pciio[PCIIO_SIZE];
};

struct pciio_dev *pciio_devp; /*设备结构体指针*/

/*文件打开函数*/
int pciio_open(struct inode *inode, struct file *filp)
{
	/*将设备结构体指针赋值给文件私有数据指针*/
	filp->private_data = pciio_devp;
	return 0;
}
/*文件释放函数*/
int pciio_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/*读函数*/
static ssize_t pciio_read(struct file *filp, char __user *buf, size_t size,
	loff_t *ppos)
{
	unsigned long p =  *ppos;/*p:偏移量*/
	unsigned int count = size;/*count:读取的大小*/
	int ret = 0;
	struct pciio_dev *dev = filp->private_data; /*获得设备结构体指针*/

	/*分析和获取有效的写长度*/
	if (p >= PCIIO_SIZE)
		return 0;/*偏移大于文件长度，停止读文件*/
	if (count > PCIIO_SIZE - p)
		count = PCIIO_SIZE - p;/*剩余部分不足欲读取的大小*/
	
	if(copy_to_user(buf, (void *)(dev->pciio + p), count))
	{
		ret =  - EFAULT;
	} 
	else
	{
		*ppos += count;	/*改变偏移量*/
		ret = count;	/*返回读取字节的大小*/
		printk(KERN_INFO "read %u bytes(s) from %lu\n", count, p);
	}
	return ret;
}

/*文件操作结构体*/
static const struct file_operations pciio_fops =
{
	.owner = THIS_MODULE,
	.read = pciio_read,
	.open = pciio_open,
	.release = pciio_release,
};

/*初始化并注册cdev*/
static void pciio_setup_cdev(struct pciio_dev *dev, int index)
{
	int err, devno = MKDEV(pciio_major, index);

	cdev_init(&dev->cdev, &pciio_fops);		/*初始化设备*/
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);	/*添加设备*/
	if (err)
		printk(KERN_NOTICE "Error");
}

/*设备驱动模块加载函数*/
int pciio_init(void)
{
	int result;
	dev_t devno = MKDEV(pciio_major, 0);/*由主次设备号合成设备编号*/
	u32 i,busno,deviceno,funcno,regval,retval,venderID,devID;
	u32 basereg;

	/* 申请设备号*/
	if (pciio_major)/*如果主设备号未被占用*/
		result = register_chrdev_region(devno, 1, "pciio");/*手工分配主设备号*/
	else
	{
		result = alloc_chrdev_region(&devno, 0, 1, "pciio");/*动态分配设备号*/
		pciio_major = MAJOR(devno);/*提取主设备号*/
	}
	if (result < 0)
		return result;

	/* 动态申请设备结构体的内存*/
	pciio_devp = kmalloc(sizeof(struct pciio_dev), GFP_KERNEL);
	if (!pciio_devp)
	{
		result = -ENOMEM;
		goto fail_malloc;
	}

	memset(pciio_devp, 0, sizeof(struct pciio_dev));	/*初始化设备结构体为0*/
	pciio_setup_cdev(pciio_devp, 0);	/*调用cdev初始化和注册函数*/

	/*初始化设备文件，向PCI配置地址寄存器写入数据，
	  从PCI配置数据寄存器读入数据，把数据保存在设备结构体的int型数组中*/
	i = 0;
	for (busno=0;busno<5;busno++)	/*扫描总线号*/
	{
   		for (deviceno=0;deviceno<32;deviceno++)/*扫描设备号*/
   		{
			for (funcno=0;funcno<8;funcno++)/*扫描功能号*/
			{
	    		regval=0x80000000+(busno<<16)+(deviceno<<11)+(funcno<<8);/*生成目标地址，从0号寄存器开始*/
	    		outl(0x0CF8,regval);	/*将目标地址写入地址寄存器*/
	    		retval=inl(0x0CFC);		/*从数据寄存器中读数据*/
	    		if(retval!=0xffffffff)	/*如果存在该PCI功能*/
	    		{	
					venderID = retval&0xffff;	/*提取供应商代码*/
					devID = (retval>>16)&0xffff;/*提取设备代码*/
					regval += 0x10;				/*跳到10H基址寄存器*/
					outl(0x0CF8, regval);
					retval=inl(0x0CFC);			/*提取第一个32位基地址寄存器内容*/
					if ((retval&0x1) == 0)		/*如果是存储器基地址*/
		    			basereg=(retval)&0xfffffff0;
					else						/*如果是I/O基地址*/
		   				basereg=(retval)&0xfffffffc;
		   			/*将各代码依次写入设备结构体的数组中*/
		   			*(pciio_devp->pciio+i*6) = venderID;
					*(pciio_devp->pciio+i*6+1) = devID;
					*(pciio_devp->pciio+i*6+2) = busno;
					*(pciio_devp->pciio+i*6+3) = deviceno;
					*(pciio_devp->pciio+i*6+4) = funcno;
					*(pciio_devp->pciio+i*6+5) = basereg;
					i++;
					if (funcno==0)
					{	/*如果是单功能设备，就不再按功能号扫描*/
		    			regval=(regval&0xfffffff0)+0x0c;
		    			outl(0x0CF8,regval);
		    			retval=inl(0x0CFC);
		    			retval=retval>>16;
		    			if ((retval&0x80)==0)/*头标类型寄存器第一位指示了是否是单功能设备*/
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

/*模块卸载函数*/
void pciio_exit(void)
{
	cdev_del(&pciio_devp->cdev);   /*注销cdev*/
	unregister_chrdev_region(MKDEV(pciio_major, 0), 1); /*释放设备号*/
}

MODULE_LICENSE("Dual BSD/GPL");
module_param(pciio_major, int, S_IRUGO);
module_init(pciio_init);
module_exit(pciio_exit);

