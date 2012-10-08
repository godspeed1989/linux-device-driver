/* 实现内存的推迟分配。在进行第一次写时才进行内存的分配。
 * 但可能会产生并发问题造成内存泄漏。因此要给数据加锁。
 */
#define  DEV_DATSEG_ORDER  (2)
#define  DEV_DATSEG_SHIFT  (PAGE_SHIFT+DEV_DATSEG_ORDER)
#define  DEV_DATSEG_SIZE   (PAGE_SIZE<<DEV_DATSEG_ORDER)
#define  DEV_DATSEG_MASK   (~(DEV_DATSEG_SIZE-1))
#define  MEM_ALLOC_FLAGS   (GFP_KERNEL|__GFP_ZERO|__GFP_HIGHMEM)
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

int dev_major;

struct gendisk * simp_blkdev;
struct request_queue * simp_blkdev_queue;
struct radix_tree_root simp_blkdev_data = RADIX_TREE_INIT(GFP_KERNEL);
DEFINE_MUTEX(simp_blkdev_data_lock);

int simp_blkdev_getgeo(struct block_device *bdev, struct hd_geometry* geo)
{
	/* capacity   heads   sectors   cylinders
	 *  0~16M       1        1       0~32768
	 * 16M~512M     1       32    1024~32768
	 * 512M~16G    32       32    1024~32768
	 *  16G~...   255       63    2088~.....
	 */
	if (DEV_CAPACITY>>20 < 16) {
		geo->heads = 1;
		geo->sectors = 1;
	}
	else if(DEV_CAPACITY>>20 < 512) {
		geo->heads = 1;
		geo->sectors = 32;
	}
	else if(DEV_CAPACITY>>20 < 16*1024) {
		geo->heads = 32;
		geo->sectors = 32;
	}
	else {
		geo->heads = 255;
		geo->sectors = 63;
	}
	geo->cylinders = DEV_CAPACITY>>9/geo->heads/geo->sectors;
	
	return 0;
}

struct block_device_operations simp_blkdev_fops = {
	.owner = THIS_MODULE,
	.getgeo = simp_blkdev_getgeo,
};

void free_disk_mem(void)
{
	struct page *pglist[64];
	unsigned long long idx;
	int i, listcnt;
	idx = 0;
	do{
		/* 一次从基树上获取多个节点进行释放 */
		listcnt = radix_tree_gang_lookup(&simp_blkdev_data, 
				(void **)pglist, idx, ARRAY_SIZE(pglist));
		for(i=0 ; i < listcnt ; i++)
		{
			idx = pglist[i]->index;
			radix_tree_delete(&simp_blkdev_data, idx);
			__free_pages(pglist[i], DEV_DATSEG_ORDER);
		}
		idx++;
	}while(listcnt == ARRAY_SIZE(pglist));
}

int simp_blkdev_trans_oneseg(struct page *start_page, 
		unsigned long offset, void *buf, unsigned int len, int dir)
{
	unsigned int count_done;
	struct page *current_pg;
	unsigned int current_off;
	unsigned int current_cnt;
	unsigned int current_left;
	void *disk_mem;
	
	count_done = 0;
	while(count_done < len)
	{
		current_pg = start_page + ((offset+count_done)>>PAGE_SHIFT);
		current_off = (offset+count_done)%PAGE_SIZE;
		current_left = (unsigned int)PAGE_SIZE-current_off;
		current_cnt = min(len-count_done, current_left);
		/* kmap一次只能映射一个高端内存页，vmap虽然可以映射多个，但太耗时。*/
		disk_mem = kmap(current_pg);
		if(!disk_mem)
		{
			printk("blkdev: get page's addr failed: %p\n", start_page);
			return -ENOMEM;
		}
		disk_mem += current_off;
		
		if(dir==0) /*READ*/
			memcpy(buf+count_done, disk_mem, current_cnt);
		else  /*WRITE*/
			memcpy(disk_mem, buf+count_done, current_cnt);
		kunmap(current_pg);
		count_done += current_cnt;
	}
	return 0;
}

int simp_blkdev_trans(unsigned long long disk_offset, void *buf,
					unsigned int len, int dir)
{
	unsigned int count_done = 0;
	unsigned int count_current = 0;
	struct page *start_page = NULL;

	while(count_done < len)
	{
		unsigned int trans_left = len - count_done;
		unsigned int ds_offset = (disk_offset+count_done)%DEV_DATSEG_SIZE;
		unsigned int ds_left = (unsigned int)DEV_DATSEG_SIZE - ds_offset;
		unsigned long ds_index = (disk_offset+count_done)>>DEV_DATSEG_SHIFT;
		
		count_current = min(trans_left, ds_left);
		mutex_lock(&simp_blkdev_data_lock);
		start_page = radix_tree_lookup(&simp_blkdev_data, ds_index);
		/* start page doesn't exist */
		if(!start_page)
		{
			if(dir == 0) /* READ: directly return zero */
			{
				memset(buf+count_done, 0, count_current);
				goto trans_done;
			}
			/* WRITE: alloc page(s), insert into radix tree */
			start_page = alloc_pages(MEM_ALLOC_FLAGS, DEV_DATSEG_ORDER);
			if(!start_page)
			{
				printk("blkdev: allocate page(s) failed\n");
				mutex_unlock(&simp_blkdev_data_lock);
				return -ENOMEM;
			}
			start_page->index = ds_index;
			if(IS_ERR_VALUE(radix_tree_insert
							(&simp_blkdev_data, ds_index, start_page)))
			{
				printk("blkdev: insert page to radix tree failed: ");
				printk("seg = %lu\n", start_page->index);
				__free_pages(start_page, DEV_DATSEG_ORDER);
				mutex_unlock(&simp_blkdev_data_lock);
				return -EIO;
			}
		}
		if(IS_ERR_VALUE(simp_blkdev_trans_oneseg(start_page, ds_offset,
						buf+count_done, count_current, dir)))
		{
			mutex_unlock(&simp_blkdev_data_lock);
			return -EIO;
		}
trans_done:
		mutex_unlock(&simp_blkdev_data_lock);
		count_done += count_current;
	}/*while*/
	return 0;
}

int simp_blkdev_make_request(struct request_queue *q, struct bio *bio)
{
	int i, dir;
	struct bio_vec * bvec;
	unsigned long long disk_offset;
	void *iovec_mem;
	
	switch(bio_rw(bio))
	{
		case READ:
		case READA:    dir = 0; break;
		case WRITE:    dir = 1; break;
		default:
			printk("blkdev: unknown val of bio_rw: %lu\n", bio_rw(bio));
			goto bio_err;
	}
	
	if ( (bio->bi_sector<<SECTOR_SHIFT)+bio->bi_size > DEV_CAPACITY )
	{
		printk("simp_blkdev: bad request: S=%lu, nS=%u\n",
				(unsigned long)bio->bi_sector, bio->bi_size);
		goto bio_err;
	}
	
	disk_offset = bio->bi_sector<<SECTOR_SHIFT;

	bio_for_each_segment(bvec, bio, i)
	{
		iovec_mem = kmap(bvec->bv_page) + bvec->bv_offset;
		if(!iovec_mem)
		{
			printk("blkdev: map iovec page failed: %p\n", bvec->bv_page);
			goto bio_err;
		}
		if(IS_ERR_VALUE(simp_blkdev_trans(disk_offset, iovec_mem, 
						bvec->bv_len, dir)))
		{
			kunmap(bvec->bv_page);
			goto bio_err;
		}
		kunmap(bvec->bv_page);
		disk_offset += bvec->bv_len;
	}
	bio_endio(bio, 0);
	return 0;
bio_err:
	bio_endio(bio, -EIO);
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
	
	strcpy(simp_blkdev->disk_name, "simp_blkdev");
	simp_blkdev->major = dev_major;
	simp_blkdev->first_minor = 0;
	simp_blkdev->fops = &simp_blkdev_fops;
	simp_blkdev->queue = simp_blkdev_queue;
	set_capacity(simp_blkdev, DEV_CAPACITY>>SECTOR_SHIFT);
	add_disk(simp_blkdev);
	return 0;

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
