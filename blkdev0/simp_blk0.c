/*
 * 最简单的块设备驱动。使用提供的I/O调度器来处理bio。
 */
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/fs.h>

#define  SECTOR_SIZE    512
#define  DEV_CAPACITY   16*1024*1024
unsigned char simp_blkdev_data[DEV_CAPACITY];
int dev_major;

struct gendisk * simp_blkdev;
struct request_queue * simp_blkdev_queue;

struct block_device_operations simp_blkdev_fops = {
	.owner = THIS_MODULE,
};

void simp_blk_do_request(struct request_queue *q)
{
	struct request *req;
	req = blk_fetch_request(q);
	while( req != NULL )
	{
		sector_t sector = blk_rq_pos(req);
		unsigned int nr_sectors = blk_rq_cur_sectors(req);
		
		if(req->cmd_type != REQ_TYPE_FS)
		{
			printk("simp_blkdev: skip non-fs request");
			blk_end_request_all(req, -EIO);
			continue;
		}
		
		if((sector+nr_sectors)<<9 > DEV_CAPACITY)
		{
			printk("simp_blkdev: bad request: S=%lu, nS=%u\n",
					(unsigned long)sector, nr_sectors);
			goto end;
		}
		
		switch(rq_data_dir(req))
		{
			case READ:
				memcpy(req->buffer, simp_blkdev_data+(sector<<9),
						nr_sectors<<9);
				break;
			case WRITE:
				memcpy(simp_blkdev_data+(sector<<9), req->buffer,
						nr_sectors<<9);
				break;
		}
end:
		if(!__blk_end_request_cur(req, 0))
			req = blk_fetch_request(q);
	}
}

int __init simp_blk_init(void)
{
	struct elevator_queue *old_e;
	/* registe blk device */
	dev_major = 0;
	dev_major = register_blkdev(dev_major, "blkdev");
	if(dev_major <= 0)
	{
		printk("blkdev: unable reg blk dev.\n");
		return -EBUSY;
	}
	/* init blkdev's request queue */
	simp_blkdev_queue = blk_init_queue(simp_blk_do_request, NULL);
	if(!simp_blkdev_queue)
	{
		return -ENOMEM;
		blk_cleanup_queue(simp_blkdev_queue);
	}
	/* replace queue's elevator type */
	old_e = simp_blkdev_queue->elevator;
	if( elevator_change(simp_blkdev_queue,"noop") )
	{
		simp_blkdev_queue->elevator = old_e;
		printk("blkdev: replace elevator failed, use default");
	}

	simp_blkdev = alloc_disk(1);
	if(!simp_blkdev)
	{
		return -ENOMEM;
	}

	strcpy(simp_blkdev->disk_name, "simp_blkdev");
	simp_blkdev->major = dev_major;
	simp_blkdev->first_minor = 0;
	simp_blkdev->fops = &simp_blkdev_fops;
	simp_blkdev->queue = simp_blkdev_queue;
	set_capacity(simp_blkdev, DEV_CAPACITY>>9);
	add_disk(simp_blkdev);

	return 0;
}
module_init(simp_blk_init);

void __exit simp_blk_exit(void)
{
	del_gendisk(simp_blkdev);
	put_disk(simp_blkdev);
	blk_cleanup_queue(simp_blkdev_queue);
}
module_exit(simp_blk_exit);
MODULE_LICENSE("GPL");
