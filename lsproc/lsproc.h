#ifndef __LS_PROC_H__
#define __LS_PROC_H__

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/list.h>

#define DEV_NUM  250
#define MOD_NAME "ls_proc"

#define DPRINTF(str, args...) printk(MOD_NAME": "str, ##args)
#define MSG() DPRINTF("%s()\n", __FUNCTION__)

static int lsproc_open(struct inode *inode, struct file *filep);
static ssize_t lsproc_read(struct file *filep,
                           char *buf, size_t count, loff_t *ppos);
static long lsproc_ioctl(struct file *filep,
                         unsigned int cmd, unsigned long arg);

#endif
