#ifndef __INSANE_H__
#define __INSANE_H__

#define DEV_NAME "insane"

#define MSG(string, args...)  printk(DEV_NAME ": " string, ##args)

const char * real_dev = "eth0";

struct insane_priv
{
	struct net_device *net_dev;
	/* real net interface device */
	struct net_device *priv_device;
	struct net_device_stats priv_stats;
};

#endif

