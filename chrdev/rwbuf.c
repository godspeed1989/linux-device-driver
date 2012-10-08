/*
* rwbuf.c driver for virtual char-device
*/
#ifndef __KERNEL__
#define __KERNEL__
#endif
#ifndef MODULE
#define MODULE
#endif

#include <linux/kernel.h>    //for kernel programming
#include <linux/module.h>    //for kernel module struct
#include <linux/fs.h>        //for struct file_operation
#include <asm/uaccess.h>     //for put_user
#include <linux/init.h>
#include <linux/semaphore.h>

#include "rwbuf.h"

static struct file_operations rwbuf_fops =
{
    owner:     THIS_MODULE,
    open:      rwbuf_open,
    release:   rwbuf_release,
    read:      rwbuf_read,
    write:     rwbuf_write,
    ioctl:     rwbuf_ioctl
};

/*
*  initial the char device
*/
#define Num 255

static int Major;
static spinlock_t spin_inuse = SPIN_LOCK_UNLOCKED;
static __init int init_chrdev(void)
{
    printk("Initial virtual char-device...\n");
    Major=register_chrdev(Num, DEVICE_NAME, &rwbuf_fops);
    if( Major<0 )
    {
        printk("Error : registe device error!!!\n");
        return -1;
    }
    printk("Major=%d,Registe Char-Device Success.\n",Major);
    return 0;
}

static __exit void exit_chrdev(void)
{
    unregister_chrdev(Num, DEVICE_NAME);
    printk("bye,13081198\n");
}
module_init(init_chrdev);
module_exit(exit_chrdev);
/*-----------------fops implitation-------------------*/

#define rwbuf_size 1024      //MAX size of buffer
static char rwbuf[rwbuf_size] = {"13081198"};
static int rwlen = 0;       //length of string
static int wr_times = 0;    //total write times
static int inuse = 0;       //one process one time
/*
*  open operation
*/
static int rwbuf_open(struct inode *inode, struct file *filep)
{
    //spin_trylock(&spin_inuse);
    if(inuse != 0)
    {
        printk("Only one process can use now!\n");
        return -1;
    }
    inuse = 1;
    //spin_unlock(&spin_inuse);
    try_module_get(THIS_MODULE);
    return 0;
}
/*
*  close operation
*/
static int rwbuf_release(struct inode *inode, struct file *filep)
{
    //spin_trylock(&spin_inuse);
    inuse = 0;
    //spin_unlock(&spin_inuse);
    module_put(THIS_MODULE);
    return 0;
}
/*
*  write operation
*/
static size_t tmp;
static char *str;
static ssize_t rwbuf_write(struct file *filep, const char *buf,
                           size_t count, loff_t *ppos)
{
    wr_times++;
    printk("write : times=%d, count=%d\n", wr_times, count);
    /*check the length of buf*/
    if(count > rwbuf_size)
    {
        printk("Warning : count=%d, too long!\n",count);
        count = 1024;
    }
    tmp = 0;
    str = (char*)buf;
    while( *str!='\0' && tmp<count)
    {
        tmp++;
        str++;
    }
    if(tmp != count)
    {
        printk("Warning : %d Write str length not match!\n", tmp);
    }
    /*copy from user space to kernel space*/
    copy_from_user(rwbuf, buf, count);
    rwlen = count;
    /*print success message*/
    printk("Wirte Success : %d chars!\n", count);    
    return count;
}
/*
*  read operation
*/
static ssize_t rwbuf_read(struct file *filep, char *buf,
                          size_t count, loff_t *ppos)
{
    printk("Prepare to read. count = %d\n", count);
    /*copy from user space to kernel space*/
    if(count == 0)
        copy_to_user((void*)buf, "13081198", 20);
    else
        copy_to_user((void*)buf, rwbuf, count);
    /*print success message*/
    printk("Read Success : %d chars!\n", count);    
    return count;
}
/*
*  ioctl operation
*/
static int rwbuf_ioctl(struct inode* inode, struct file *filep,
                       unsigned int cmd, unsigned long arg)
{
    switch(cmd)
    {
        case RW_CLEAR:
            rwlen = 0; //clear buf by set its len zero
            printk("rwbuf in kernel zero-ed.\n");
            break;
        case RW_LENGTH:
            return rwlen;
            break;
        case RW_TIMES:
            return wr_times;
            break;          
    }
    return 0;
}

