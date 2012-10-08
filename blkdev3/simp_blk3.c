/* 模块被加载在非线性映射区，使用数组实现simp_blkdev_data占用大量非线性映射区域。
 * 为节约非线性映射区，改为动态申请内存页，通过基树（radix tree）进行管理。
 * 内存分配方式：每次(2^DEV_DATSEG_ORDER大小)页的内存段进行。
 * 由于使用__get_free_pages申请内存，这里申请的均为低端内存，无须映射即可使用。
 */
#define  DEV_DATSEG_ORDER  (2)
#define  DEV_DATSEG_SHIFT  (PAGE_SHIFT+DEV_DATSEG_ORDER)
#define  DEV_DATSEG_SIZE   (PAGE_SIZE<<DEV_DATSEG_ORDER)
#define  DEV_DATSEG_MASK   (~(DEV_DATSEG_SIZE-1))

#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/gfp.h>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <linux/radix-tree.h>

#define  MAX_PARTITIONS   5
#define  SECTOR_SHIFT     9
#define  SECTOR_SIZE      (1ULL<<SECTOR_SHIFT)
#define  SECTOR_MASK      (~(SECTOR_SIZE-1))
#define  DEV_CAPACITY     16*1024*1024

int alloc_disk_mem(void);
void free_disk_mem(void);
struct radix_tree_root simp_blkdev_data;
int dev_major;

struct gendisk * simp_blkdev;
struct request_queue * simp_blkdev_queue;

#define for_each_data_seg(i) \
	for(i=0;i<((DEV_CAPACITY+DEV_DATSEG_SIZE-1)>>DEV_DATSEG_SHIFT);i++)

int alloc_disk_mem(void)
{
	int ret, i;
	void *pg;
	INIT_RADIX_TREE(&simp_blkdev_data, GFP_KERNEL);
	for_each_data_seg(i)
	{
		pg = (void*)__get_free_pages(GFP_KERNEL|__GFP_ZERO, DEV_DATSEG_ORDER);
		if(!pg)
		{
			ret = -ENOMEM;
			goto err_alloc;
		}
		ret = radix_tree_insert(&simp_blkdev_data, i, pg);
		if(IS_ERR_VALUE(ret))
			goto err_insert_radix_tree;
	}
	return 0;
err_insert_radix_tree:
	free_pages((unsigned long)pg, DEV_DATSEG_ORDER);
err_alloc:
	free_disk_mem();
	return ret;
}

void free_disk_mem(void)
{
	int i;
	void *pg;
	for_each_data_seg(i)
	{
		pg = radix_tree_lookup(&simp_blkdev_data, i);
		radix_tree_delete(&simp_blkdev_data, i);
		/*free NULL is safe*/
		free_pages((unsigned long)pg, DEV_DATSEG_ORDER);
	}
}

struct block_device_operations simp_blkdev_fops = {
	.owner = THIS_MODULE,
};

int simp_blkdev_make_request(struct request_queue *q, struct bio *bio)
{
	int i;
	struct bio_vec * bvec;
	unsigned long long disk_offset;
	if ((bio->bi_sector<<SECTOR_SHIFT)+bio->bi_size > DEV_CAPACITY)
	{
		printk("simp_blkdev: bad request: S=%lu, nS=%u\n",
				(unsigned long)bio->bi_sector, bio->bi_size);
		bio_endio(bio, -EIO);
		return 0;
	}
	
	disk_offset = bio->bi_sector<<SECTOR_SHIFT;
	
	/* bio_vec对应的数据长度只有一个页面；虽然可能页面越界，但不会跨越2个以上页面。
	 * 为了更好地兼容性，我们让代码支持2个以上的页面。 */
	bio_for_each_segment(bvec, bio, i)
	{
		unsigned int count_done = 0;
		unsigned int count_current = 0;
		void *iovec_mem = NULL, *disk_mem = NULL;

		iovec_mem = kmap(bvec->bv_page) + bvec->bv_offset;
		while(count_done < bvec->bv_len)
		{
			unsigned int bv_left = bvec->bv_len - count_done;
			unsigned int ds_offset = (disk_offset+count_done)%DEV_DATSEG_SIZE;
			unsigned int ds_left = (unsigned int)DEV_DATSEG_SIZE - ds_offset;
			unsigned long ds_index = (disk_offset+count_done)>>DEV_DATSEG_SHIFT;
			
			count_current = min(bv_left, ds_left);

			disk_mem = radix_tree_lookup(&simp_blkdev_data, ds_index);
			if(!disk_mem)
			{
				printk("blkdev: search mem failed: %lu\n", ds_index);
				kunmap(bvec->bv_page);
				bio_endio(bio, -EIO);
				return 0;
			}
			disk_mem += ds_offset;
			
			switch(bio_rw(bio)) 
			{
				case READ:
				case READA:/*read ahead*/
					memcpy(iovec_mem+count_done, disk_mem, count_current);
					break;
				case WRITE:
					memcpy(disk_mem, iovec_mem+count_done, count_current);
					break;
				default:
					printk("blkdev: unknow bio type\n");
					kunmap(bvec->bv_page);
					bio_endio(bio, -EIO);
					return 0;
			}
			count_done += count_current;
		}/*while (count_done < bvec->bv_len)*/
		kunmap(bvec->bv_page);
		disk_offset += bvec->bv_len;
	}/*bio_for_each_segment*/
	bio_endio(bio, 0);
	
	return 0;
}

int __init simp_blk_init(void)
{
	int ret;
	dev_major = 0;
	dev_major = register_blkdev(dev_major, "blkdev");
	if(dev_major <= 0) {
		printk("blkdev: unable reg blk dev.\n");
		return -EBUSY;
	}

	simp_blkdev_queue = blk_alloc_queue(GFP_KERNEL);
	if(!simp_blkdev_queue) {
		return -ENOMEM;
	}
	/*deal with request ourselves, without I/O sched*/
	blk_queue_make_request(simp_blkdev_queue,
						simp_blkdev_make_request);

	simp_blkdev = alloc_disk(MAX_PARTITIONS);
	if(!simp_blkdev) {
		ret = -ENOMEM;
		goto err_alloc_disk;
	}
	
	ret = alloc_disk_mem();
	if(IS_ERR_VALUE(ret))
		goto err_alloc_disk_mem;
	
	strcpy(simp_blkdev->disk_name, "simp_blkdev");
	simp_blkdev->major = dev_major;
	simp_blkdev->first_minor = 0;
	simp_blkdev->fops = &simp_blkdev_fops;
	simp_blkdev->queue = simp_blkdev_queue;
	set_capacity(simp_blkdev, DEV_CAPACITY>>SECTOR_SHIFT);
	add_disk(simp_blkdev);
	return 0;

err_alloc_disk_mem:
	put_disk(simp_blkdev);
err_alloc_disk:
	blk_cleanup_queue(simp_blkdev_queue);
	return ret;
}
module_init(simp_blk_init);

void __exit simp_blk_exit(void)
{
	del_gendisk(simp_blkdev);
	free_disk_mem();
	put_disk(simp_blkdev);
	blk_cleanup_queue(simp_blkdev_queue);
}
module_exit(simp_blk_exit);
MODULE_LICENSE("GPL");
