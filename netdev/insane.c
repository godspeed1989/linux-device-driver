/**
 * fake network interface device
 */
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/neighbour.h>
#include <linux/if_arp.h>
#include <net/net_namespace.h>
#include "insane.h"

int insane_open(struct net_device *dev)
{
	MSG("netdev open\n");
	return 0;
}

int insane_stop(struct net_device *dev)
{
	MSG("netdev close\n");
	return 0;
}

netdev_tx_t insane_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct insane_priv *idev = container_of(dev, struct insane_priv, net_dev);

	if(idev->priv_device)
	{
		idev->priv_stats.tx_packets++;
		idev->priv_stats.tx_bytes += skb->len;
		skb->dev = idev->priv_device;
		skb->priority = 1;
		dev_queue_xmit(skb);
		return NETDEV_TX_OK;
	}
	// real net interface is not availiable
	idev->priv_stats.tx_dropped++;
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

struct net_device_stats* insane_get_stats(struct net_device *dev)
{
	struct insane_priv *idev;
	idev = container_of(dev, struct insane_priv, net_dev);
	return &idev->priv_stats;
}

int insane_init(struct net_device *dev)
{
	struct net_device *slave;
	struct insane_priv *idev;
	MSG("netdev init\n");
	idev = container_of(dev, struct insane_priv, net_dev);

	// don't forget use dev_put() to release slave
	slave = dev_get_by_name(&init_net, real_dev);
	if(!slave)
	{
		MSG("no %s dev exist\n", real_dev);
		return -ENODEV;
	}
	if(slave->type != ARPHRD_ETHER && slave->type != ARPHRD_LOOPBACK)
	{
		MSG("%s is not ethernet device\n", real_dev);
		dev_put(slave);
		return -EINVAL;
	}
    MSG("capture real net dev\n");
    // setup header_ops, type, hard_header_len, addr_len, mtu, etc.
	ether_setup(dev);
	idev->priv_device = slave;
	
	memcpy(dev->dev_addr, slave->dev_addr, sizeof(slave->dev_addr));
	memcpy(dev->broadcast, slave->broadcast, sizeof(slave->broadcast));
	return 0;
}

/**
 * variables and init&exit functions
 */
static struct net_device_ops insane_ops =
{
	.ndo_init        = insane_init,
	.ndo_open        = insane_open,
	.ndo_stop        = insane_stop,
	.ndo_start_xmit  = insane_xmit,
	.ndo_get_stats   = insane_get_stats,
};

static struct insane_priv insane_dev =
{
	.net_dev =
	{
		.name       = "insane",
		.netdev_ops = &insane_ops,
	},
	.priv_device = NULL,   // setup at init
};

int __init init_insane_module(void)
{
	if(register_netdev(&insane_dev.net_dev))
	{
		MSG("can't register\n");
		return -EIO;
	}
	return 0;
}
module_init(init_insane_module);

void __exit exit_insane_module(void)
{
	unregister_netdev(&insane_dev.net_dev);
	dev_put(insane_dev.priv_device);
}
module_exit(exit_insane_module);
MODULE_LICENSE("GPL");

