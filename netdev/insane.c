/**
 * fake network interface device
 */
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/neighbour.h>
#include <linux/if_arp.h>
#include <net/net_namespace.h>
#include "insane.h"

static int insane_open(struct net_device *dev)
{
	MSG("netdev open\n");
	netif_start_queue(dev);
	return 0;
}

static int insane_stop(struct net_device *dev)
{
	MSG("netdev close\n");
	if (!netif_queue_stopped(dev))
		netif_stop_queue(dev);;
	return 0;
}

static netdev_tx_t insane_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct insane_priv *idev;
	idev = netdev_priv(dev);

	if(idev->priv_device)
	{
		idev->priv_stats.tx_packets++;
		idev->priv_stats.tx_bytes += skb->len;
		skb->dev = idev->priv_device;
		dev_queue_xmit(skb);
		return NETDEV_TX_OK;
	}
	// real net interface is not availiable
	idev->priv_stats.tx_dropped++;
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

static struct net_device_stats* insane_get_stats(struct net_device *dev)
{
	struct insane_priv *idev;
	MSG("netdev get stats\n");
	idev = netdev_priv(dev);
	return &idev->priv_stats;
}

static int insane_init(struct net_device *dev)
{
	struct net_device *slave;
	struct insane_priv *idev;
	MSG("netdev init\n");
	idev = netdev_priv(dev);

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
	MSG("captured real net dev\n");
	// setup header_ops, type, hard_header_len, addr_len, mtu, etc.
	ether_setup(dev);
	idev->priv_device = slave;

	memcpy(dev->dev_addr, slave->dev_addr, sizeof(dev->dev_addr));
	memcpy(dev->broadcast, slave->broadcast, sizeof(dev->broadcast));
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

static void insane_setup(struct net_device *dev)
{
	MSG("netdev setup\n");
	dev->netdev_ops = &insane_ops;
	dev->flags = 0;
}

static struct insane_priv *insane_dev;

static int __init init_insane_module(void)
{
	int ret;
	struct net_device *ndev;

	ndev = alloc_netdev(sizeof(struct insane_priv), DEV_NAME, insane_setup);
	if(!ndev)
		return -ENOMEM;

	insane_dev = netdev_priv(ndev);
	insane_dev->priv_device = NULL;

	ret = register_netdev(ndev);
	if(ret)
	{
		MSG("can't register\n");
		free_netdev(ndev);
		return ret;
	}
	MSG("register succeed\n");
	return 0;
}
module_init(init_insane_module);

static void __exit exit_insane_module(void)
{
	struct net_device *ndev;
	if(insane_dev)
	{
		ndev = insane_dev->priv_device;
		if(ndev)
			dev_put(ndev);
		ndev = insane_dev->net_dev;
		if(ndev)
		{
			if(netif_running(ndev))
			{
				MSG("netif running, detach it\n");
				netif_device_detach(ndev);
			}
			unregister_netdev(ndev);
			free_netdev(ndev);
		}
	}
}
module_exit(exit_insane_module);
MODULE_LICENSE("GPL");

