#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <asm/uaccess.h> /* copy_from_user() */

/* Direcotry/File, which we will create */
struct dentry *dir, *file_ret;

static unsigned int req_count = 0;

#define _M_ "yl_debug"

static ssize_t test_read(struct file *fp, char __user *user_buf, size_t count, loff_t *pos)
{
    char buf[24] = {0};
    printk(KERN_INFO _M_ ": read request from Userspace!\n");
    ++req_count;
    snprintf(buf, sizeof(buf), "%u\n", req_count);

    return simple_read_from_buffer(user_buf, count, pos, buf, sizeof(buf));
}

static ssize_t test_write(struct file *fp, const char __user *user_buf, size_t count, loff_t *pos)
{
    char buf[24] = {0};
    unsigned int len;

    printk(KERN_INFO _M_ ": write request from Userspace!\n");
    len = sizeof(buf) - 1;
    if( copy_from_user(buf, user_buf, len) )
        return -EFAULT;
    buf[len] = '\0';

    req_count = (unsigned int)simple_strtol(buf, NULL, 10);
    return count;
}

/* File ops */
static const struct file_operations test_fops = {
    .read = test_read,
    .write = test_write
};

static int __init test_init(void)
{
    /* create a dicreoty: /sys/kernel/debug/yl_debug */
    dir = debugfs_create_dir(_M_, NULL);
    if (dir == NULL) {
        printk(KERN_ERR _M_ ": failed to create directory!\n");
        return -ENODEV;
    }

    file_ret = debugfs_create_file("my_info", 0644, dir, NULL, &test_fops);
    if(file_ret == NULL)
        goto func_err;

    return 0;
func_err:
    debugfs_remove_recursive(dir);
    printk(KERN_ERR _M_ ": failed to create file!\n");
    return -ENODEV;
}

static void __exit test_exit(void)
{
    debugfs_remove_recursive(dir);
    printk(KERN_INFO _M_ ": Bye, debugfs world!\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yanlin");

module_init(test_init);
module_exit(test_exit);

