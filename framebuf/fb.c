#include <linux/module.h>
#include <linux/fb.h>

#define MSG(string, args...)  printk("small-fb: " string, ##args)

#define FB_WIDTH           320
#define FB_HEIGHT          240
#define FB_BPP               3  /* bytes per pixel */
#define FB_STRIDE         1024
#define FB_BASE     0xf8200000

static struct fb_ops fb_ops =
{
	.fb_check_var   = NULL,
	.fb_set_par     = NULL,
	.fb_setcolreg   = NULL,
	.fb_pan_display = NULL,
	.fb_mmap        = NULL,
	/* these are mandatory and implemented elsewhere */
	.fb_fillrect    = cfb_fillrect,
	.fb_copyarea    = cfb_copyarea,
	.fb_imageblit   = cfb_imageblit,
};

static struct fb_var_screeninfo fb_var =
{
	.xres = FB_WIDTH,
	.yres = FB_HEIGHT,
	.xres_virtual = FB_WIDTH,
	.yres_virtual = FB_HEIGHT,
	.red   = {16,8,},
	.green = { 8,8,},
	.blue  = { 0,8,},
	.bits_per_pixel = 8 * FB_BPP,
	.vmode = FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo fb_fix =
{
	.id = "small-FB",
	.type =  FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_TRUECOLOR,
	.line_length = FB_STRIDE * FB_BPP,
	.smem_start = FB_BASE,
	.smem_len = FB_HEIGHT * FB_STRIDE * FB_BPP,
	.accel = FB_ACCEL_NONE,
};

static struct fb_info fb_info =
{
	.fbops = &fb_ops,
	.screen_base = 0 /* to be compiled at load time */,
	.screen_size = FB_STRIDE * FB_HEIGHT * FB_BPP,
	.state = FBINFO_STATE_RUNNING,
};

int __init fb_init(void)
{
	int ret;
	void __iomem *addr;

	fb_info.fix = fb_fix;
	fb_info.var = fb_var;
	addr = ioremap(FB_BASE, fb_info.screen_size);
	if (!addr)
		return -ENOMEM;
	fb_info.screen_base = addr;

	ret = register_framebuffer(&fb_info);
	if (ret)
	{
		iounmap(addr);
		return -ENOMEM;
	}
	else
		MSG("registered\n");
	return 0;
}
module_init(fb_init);

void __init fb_exit(void)
{
	unregister_framebuffer(&fb_info);
	iounmap(fb_info.screen_base);
}
module_exit(fb_exit);
MODULE_LICENSE("GPL");

