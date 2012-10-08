//rwbuf.h driver for virtual char-device
MODULE_LICENSE("GPL");
MODULE_AUTHOR("13081198");


#define DEVICE_NAME "ChrDev_A"

#define RW_CLEAR  0x909090  /*清除设备中写入的字符串*/
#define RW_LENGTH 0x808080  /*返回设备中有效数据的长度*/
#define RW_TIMES  0x707070  /*返回设备被写入的次数*/

/*functions declaration*/
static __init  int init_chrdev(void);
static __exit void exit_chrdev(void);

static int rwbuf_open(struct inode *inode, struct file *filep);
static int rwbuf_release(struct inode *inode, struct file *filep);
static ssize_t  rwbuf_read(struct file *filep, char *buf,
                           size_t count, loff_t *ppos);
static ssize_t rwbuf_write(struct file *filep, const char *buf,
                           size_t count, loff_t *ppos);
static int rwbuf_ioctl(struct inode* inode, struct file *filep,
                       unsigned int cmd, unsigned long arg);



