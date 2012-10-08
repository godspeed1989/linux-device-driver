/* 
 * 未使用I/O调度器来处理bio，摆脱了I/O调度器和通用的__make_request对bio的处理。
 * 对大多的物理磁盘块设备来说，使用合适的I/O调度器更能提高性能。
 */
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/gfp.h>
#include <linux/fs.h>

#define  MAX_PARTITIONS   4
#define  SECTOR_SIZE      512
#define  DEV_CAPACITY     16*1024*1024
unsigned char simp_blkdev_data[DEV_CAPACITY];
int dev_major;

struct gendisk * simp_blkdev;
struct request_queue * simp_blkdev_queue;

struct block_device_operations simp_blkdev_fops = {
	.owner = THIS_MODULE,
};

int simp_blkdev_make_request(struct request_queue *q, struct bio *bio)
{
	struct bio_vec * bvec;
	int i;
	void *disk_mem;
	if( (bio->bi_sector<<9)+bio->bi_size > DEV_CAPACITY )
	{
		printk("simp_blkdev: bad request: S=%lu, nS=%u\n",
				(unsigned long)bio->bi_sector, bio->bi_size);
		bio_endio(bio, -EIO);
		return 0;
	}
	
	disk_mem = simp_blkdev_data + (bio->bi_sector<<9);
	
	bio_for_each_segment(bvec, bio, i)
	{
		void *iovec_mem;
		switch(bio_rw(bio)) 
		{
			case READ:
			case READA:/*预读*/
				iovec_mem = kmap(bvec->bv_page) + bvec->bv_offset;
				memcpy(iovec_mem, disk_mem, bvec->bv_len);
				kunmap(bvec->bv_page);
				break;
			case WRITE:
				iovec_mem = kmap(bvec->bv_page) + bvec->bv_offset;
				memcpy(disk_mem, iovec_mem, bvec->bv_len);
				kunmap(bvec->bv_page);
				break;
			default:
				printk("blkdev: unspported bio type\n");
				bio_endio(bio, -EIO);
				return 0;
		}
		disk_mem += bvec->bv_len;
	}
	bio_endio(bio, 0);
	return 0;
}

int __init simp_blk_init(void)
{
	dev_major = 0;
	dev_major = register_blkdev(dev_major, "blkdev");
	if(dev_major <= 0)
	{
		printk("blkdev: unable reg blk dev.\n");
		return -EBUSY;
	}

	simp_blkdev_queue = blk_alloc_queue(GFP_KERNEL);
	if(!simp_blkdev_queue)
	{
		return -ENOMEM;
	}
	/*deal with request ourselves, without I/O sched*/
	blk_queue_make_request(simp_blkdev_queue, simp_blkdev_make_request);

	simp_blkdev = alloc_disk(MAX_PARTITIONS);
	if(!simp_blkdev) {
		return -ENOMEM;
		blk_cleanup_queue(simp_blkdev_queue);
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
