/*
* proc.c driver for operate on proc fs
*/
#ifndef __KERNEL__
#define __KERNEL__
#endif
#ifndef MODULE
#define MODULE
#endif

#include <linux/kernel.h>    //for kernel programming
#include <linux/module.h>    //for kernel module struct
#include <linux/proc_fs.h>   //for proc file operation
#include <asm/uaccess.h>	 //for put_user
#include "proc.h"

#define STRINGLEN 1024
static char global_buffer[STRINGLEN];
struct proc_dir_entry *example_dir, *hello_file, *current_file, *symlink;

/*
* initial the module
*/
int init_module()
{
    /* create the dir /proc/proc_test */
    example_dir = proc_mkdir("proc_test", NULL);
    //example_dir->owner = THIS_MODULE;
    
    /* create a ro file current at /proc/proc_test/ */
    current_file = create_proc_read_entry("current", 0666, example_dir,
                                           proc_read_current, NULL);
    //current_file->owner = THIS_MODULE;
    
    /* create a wr file hello at /proc/proc_test/ */
    hello_file = create_proc_entry("hello", 0644, example_dir);
    strcpy(global_buffer, "hello");
    hello_file->read_proc = proc_read_hello;
    hello_file->write_proc = proc_write_hello;
    //hello_file->owner = THIS_MODULE;
    
    /* create a link at /proc/proc_test/ , current_too -> current */
    symlink = proc_symlink("current_too", example_dir, "current");
    //symlink->owner = THIS_MODULE;
    
    return 0;
}

/*
* cleanup the module
*/
void cleanup_module()
{
    remove_proc_entry("current_too", example_dir);
    remove_proc_entry("hello", example_dir);
    remove_proc_entry("current", example_dir);
    remove_proc_entry("proc_test", NULL);
}

/*
* read the current file operation.
* current is read-only entry, so we just need read function
*/
int proc_read_current(char *page, char **start, off_t off, int count,
                      int *eof, void *data)
{
    int len;
    try_module_get(THIS_MODULE);//increase the count of module
 
    len = sprintf(page, "current process usage:\n"
                        "name : %s\ngid : %d\nuid : %d\n",
                        current_file->name, current_file->gid, current_file->uid);
 
    module_put(THIS_MODULE);//decrease the count of module
    return len;
}
/*
* read the hello file operation
*/
int proc_read_hello(char *page, char **start, off_t off, int count,
                      int *eof, void *data)
{
    int len;
    try_module_get(THIS_MODULE);//increase the count of module
 
    len = sprintf(page, "hello message:\n"
                        "%s write: %s\n",
                        hello_file->name, global_buffer);
 
    module_put(THIS_MODULE);//decrease the count of module
    return len;
}
/*
* write the hello file operation
*/
int proc_write_hello(struct file *fiel, const char *buffer, 
                     unsigned long count,void *data)
{
    int len;
    try_module_get(THIS_MODULE);//increase the count of module

    if(count>=STRINGLEN)
    {
        len = STRINGLEN-1;//count shouldn't longer than buffer length
    }
    else
    {
        len = count;
    }
    copy_from_user(global_buffer, buffer, len);
    global_buffer[len] = '\0';

    module_put(THIS_MODULE);//decrease the count of module
    return len;
}


