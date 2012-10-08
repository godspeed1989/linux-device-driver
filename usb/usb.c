#include <linux/module.h>
#include <linux/smp_lock.h>
#include <linux/usb.h>
#include <linux/slab.h>

#define SKEL_USB_MINOR_BASE   192
/* usb device id */
#define SKEL_VENDOR_ID      0xfff0
#define SKEL_PRODUCT_ID     0xfff0
struct usb_device_id skel_usb_table[] = {
	{ USB_DEVICE(SKEL_VENDOR_ID, SKEL_PRODUCT_ID) },
	{}
};
/* usb driver object */
int skel_usb_probe(struct usb_interface*, const struct usb_device_id*);
void skel_usb_discon(struct usb_interface *intf);
struct usb_driver skel_usb_driver = {
	.name = "skeleton",
	.id_table = skel_usb_table,
	.probe = skel_usb_probe,
	.disconnect = skel_usb_discon,
};
/* skel usb driver struct */
struct skel_usb_dev {
	struct usb_device *udev;
	struct usb_interface *intf;
	unsigned char *bulk_in_buffer;/*buffer to receive data*/
	size_t bulk_in_buf_size;
	__u8 bulk_in_endpoint_addr;
	__u8 bulk_out_endpoint_addr;
	struct kref kref;
};

/*clean up: used by probe(), release(), disconnect()*/
void skel_usb_cleanup(struct kref *kref)
{
	struct skel_usb_dev *dev = container_of(kref, struct skel_usb_dev, kref);
	usb_put_dev(dev->udev);
	kfree (dev->bulk_in_buffer);
	kfree (dev);
}

int skel_usb_open(struct inode* inode, struct file *file)
{
	int retval = 0, subminor;
	struct skel_usb_dev *dev;
	struct usb_interface *intf;
	
	subminor = iminor(inode);
	/*get usb dev interface*/
	intf = usb_find_interface(&skel_usb_driver, subminor);
	if(!intf) {
		printk("simpusb: can't find dev for minor\n");
		retval = -ENODEV;
		goto exit;
	}
	/*get dev data from interface*/
	dev = usb_get_intfdata(intf);
	if(!dev) {
		retval = -ENODEV;
		goto exit;
	}
	kref_get(&dev->kref);
	file->private_data = dev; /*save our object in file's private data*/
exit:
	return retval;
}

int skel_usb_release(struct inode *inode, struct file* file)
{
	struct skel_usb_dev *dev;
	dev = (struct skel_usb_dev*)file->private_data;
	if(dev == NULL)
		return -ENODEV;
	kref_put(&dev->kref, skel_usb_cleanup);
	return 0;
}

ssize_t skel_usb_read(struct file *file, char __user *buf,
										size_t len, loff_t *ppos)
{
	struct skel_usb_dev *dev;
	int retval = 0;
	
	dev = (struct skel_usb_dev*)file->private_data;
	/* do a 'blocking' bulk read from dev */
	retval = usb_bulk_msg(dev->udev,
				usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpoint_addr),
				dev->bulk_in_buffer, min(dev->bulk_in_buf_size, len),
				&len, HZ*10);
	if(!retval)
	{
		if(!copy_to_user(buf, dev->bulk_in_buffer, len))
			retval = len;
		else
			retval = -EFAULT;
	}
	return retval;
}

void skel_usb_write_callback(struct urb *urb)
{
	if( urb->status &&
		urb->status != -ENOENT &&
		urb->status != -ECONNRESET &&
		urb->status != -ESHUTDOWN )
	{
		printk("simpusb: nonzero bulk write status : %d\n", urb->status);
	}
	usb_free_coherent(urb->dev, urb->transfer_buffer_length,
						urb->transfer_buffer, urb->transfer_dma);
}

ssize_t skel_usb_write(struct file *file, const char __user *u_buf,
										size_t len, loff_t *ppos)
{
	int retval = 0;
	struct skel_usb_dev *dev;
	struct urb *urb = NULL;
	void *buf = NULL;
	
	dev = (struct skel_usb_dev*)file->private_data;
	
	if(len == 0)
		return 0;
	
	urb = usb_alloc_urb(0, GFP_KERNEL);
	if(!urb) {
		retval = -ENOMEM;
		goto err;
	}
	
	buf = usb_alloc_coherent(dev->udev, len, GFP_KERNEL, &urb->transfer_dma);
	if(!buf) {
		retval = -ENOMEM;
		goto err;
	}
	
	if(copy_from_user(buf, u_buf, len)) {
		retval = -EFAULT;
		goto err;
	}
	
	usb_fill_bulk_urb(urb, dev->udev,
			usb_sndbulkpipe(dev->udev, dev->bulk_out_endpoint_addr),
			buf, len, skel_usb_write_callback, dev);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	/* send data out the bulk port */
	retval = usb_submit_urb(urb, GFP_KERNEL);
	if(retval) {
		printk("simpusb: failed submitting write urb: %d\n", retval);
		goto err;
	}
	usb_free_urb(urb);
	return len;
err:
	usb_free_coherent(dev->udev, len, buf, urb->transfer_dma);
	usb_free_urb(urb);
	kfree(buf);
	return retval;
}

struct file_operations skel_usb_fops = {
	.open = skel_usb_open,
	.release = skel_usb_release,
	.read = skel_usb_read,
};

/* usb driver class: used by probe */
struct usb_class_driver skel_usb_class = {
	.name = "usb/skel%d",
	.fops = &skel_usb_fops, /*file operations*/
	.minor_base = SKEL_USB_MINOR_BASE,
};

int skel_usb_probe(struct usb_interface *intf, 
					const struct usb_device_id *id)
{
	int i, retval;
	struct skel_usb_dev *dev;
	struct usb_host_interface *host_intf_desc;
	struct usb_endpoint_descriptor *ep_desc;

	/* alloc and init 'skel usb driver object' */
	dev = kmalloc(sizeof(struct skel_usb_dev), GFP_KERNEL);
	if(!dev) {
		printk("simpusb: alloc struct skel_usb_dev failed\n");
		retval = -ENOMEM;
		goto err;
	}
	memset(dev, 0, sizeof(*dev));
	kref_init(&dev->kref);
	dev->intf = intf;
	dev->udev = usb_get_dev(interface_to_usbdev(intf));
	
	host_intf_desc = intf->cur_altsetting;
	/* for each end point of the interface */
	for(i=0 ; i < host_intf_desc->desc.bNumEndpoints ; i++)
	{
		ep_desc = &host_intf_desc->endpoint[i].desc;
		/* it is a bulk in point*/
		if( !dev->bulk_in_endpoint_addr &&
			(ep_desc->bEndpointAddress & USB_DIR_IN) &&
			(ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
				==USB_ENDPOINT_XFER_BULK )
		{
			dev->bulk_in_buf_size = ep_desc->wMaxPacketSize;
			dev->bulk_in_endpoint_addr = ep_desc->bEndpointAddress;
			dev->bulk_in_buffer = kmalloc(dev->bulk_in_buf_size, GFP_KERNEL);
			if(!dev->bulk_in_buffer) {
				printk("simpusb: alloc bulk_in_buffer failed\n");
				retval = -ENOMEM;
				goto err;
			}
		}
		/* it is a bulk out point*/
		if( !dev->bulk_in_endpoint_addr &&
			(ep_desc->bEndpointAddress & USB_DIR_OUT) &&
			(ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
				==USB_ENDPOINT_XFER_BULK )
		{
			dev->bulk_out_endpoint_addr = ep_desc->bEndpointAddress;
		}
	}/*for each intf*/
	if(!dev->bulk_in_endpoint_addr && !dev->bulk_out_endpoint_addr) {
		retval = -ENOENT;
		printk("simpusb: can find neither bulk-in nor bulk-out endpoint\n");
		goto err;
	}
	
	/* save our data pointer in this interface device */
	usb_set_intfdata(intf, dev);
	
	retval = usb_register_dev(intf, &skel_usb_class);
	if(retval) {
		printk("simpusb: unable to get minor for this dev\n");
		usb_set_intfdata(intf, NULL);
		goto err;
	}
	printk("simpusb: device attached to USBkel-%d", intf->minor);
	
	return 0;

err:
	if(dev)
		kref_put(&dev->kref, skel_usb_cleanup);
	return retval;
}

void skel_usb_discon(struct usb_interface *intf)
{
	struct skel_usb_dev *dev;
	int minor = intf->minor;
	lock_kernel(); /*prevent open() from racing disconnect()*/
	
	dev = usb_get_intfdata(intf);
	usb_set_intfdata(intf, NULL);
	usb_deregister_dev(intf, &skel_usb_class);
	
	unlock_kernel();
	kref_put(&dev->kref, skel_usb_cleanup);
	printk("simpusb: %d usb dev disconnected\n", minor);
}

int __init usb_skel_init(void)
{
	int ret;
	ret = usb_register(&skel_usb_driver);
	if(ret)
		printk("simpusb:register failed errno %d", ret);
	return ret;
}
module_init(usb_skel_init);
void __exit usb_skel_exit(void)
{
	usb_deregister(&skel_usb_driver);
}
module_exit(usb_skel_exit);
MODULE_LICENSE("GPL");
