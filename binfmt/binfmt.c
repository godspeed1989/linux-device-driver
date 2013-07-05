#include <linux/module.h>
#include <linux/binfmts.h>
#include <linux/version.h>
#include <linux/file.h>
#include <linux/fs.h>

const char * IMG_VIEWER  = "/usr/bin/eog";
const char * VIEWER_ARGS = "-f";

#define MSG(string, args...)  printk("binfmt: " string, ##args);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
static int load_binary_file(struct linux_binprm *bprm)
#else
static int load_binary_file(struct linux_binprm *bprm, struct pt_regs *regs) 
#endif
{
	int retval;
	struct file* file;	
	/* Check file header information */
	if(bprm->buf[1] != 'P' || bprm->buf[2] != 'N' || bprm->buf[3] != 'G')
		return -ENOEXEC;
	MSG("filename: %s\n", bprm->filename);
	MSG("interp: %s\n", bprm->interp);

	/* Release the image file */
	fput(bprm->file);
	bprm->file = NULL;

	/*
	 * Prepare the argv for exectable file
	 * This is done in reverse order.
	 */
	retval = remove_arg_zero(bprm);
	if (retval < 0) return retval;
	// argv[2], image file path
	retval = copy_strings_kernel(1, &bprm->interp, bprm);
	if (retval < 0) return retval;
	bprm->argc++;
	// argv[1], execute args
	retval = copy_strings_kernel(1, &VIEWER_ARGS, bprm);
	if (retval < 0) return retval;
	bprm->argc++;
	// argv[0], exectable file path
	retval = copy_strings_kernel(1, &IMG_VIEWER, bprm);
	if (retval < 0) return retval;
	bprm->argc++;

	/* Change interpreter */
	retval = bprm_change_interp(IMG_VIEWER, bprm);
	if (retval < 0) return retval;

	/* Open image viewer */
	file = open_exec(IMG_VIEWER);
	if (IS_ERR(file))
		return PTR_ERR(file);
	bprm->file = file;

	/* OK. restart the process with the viewer's dentry. */
	retval = prepare_binprm(bprm);
	if(retval < 0)
		return retval;
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
	return search_binary_handler(bprm);
#else
	return search_binary_handler(bprm, regs);
#endif
}

static struct linux_binfmt binfmt_format = 
{
	.module      = THIS_MODULE,
	.load_binary = load_binary_file,
};

static int __init init_binfmt(void)
{
	register_binfmt(&binfmt_format);
	return 0;
}
module_init(init_binfmt);

static void __exit exit_binfmt(void)
{
	unregister_binfmt(&binfmt_format);
}
module_exit(exit_binfmt);

MODULE_LICENSE("GPL");


