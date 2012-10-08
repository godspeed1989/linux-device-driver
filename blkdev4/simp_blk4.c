/* simp_blkdev_make_request()将会被细化。
 * 低端内存数量少且重要，我们要改为从高端内存申请内存页。
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

int dev_major;

void free_disk_mem(void);

struct radix_tree_root simp_blkdev_data;
struct gendisk * simp_blkdev;
struct request_queue * simp_blkdev_queue;

#define for_each_data_seg(i) \
	for(i=0;i<((DEV_CAPACITY+DEV_DATSEG_SIZE-1)>>DEV_DATSEG_SHIFT);i++)

int alloc_disk_mem(void)
{
	int ret, i;
	struct page *pg;
	INIT_RADIX_TREE(&simp_blkdev_data, GFP_KERNEL);
	for_each_data_seg(i)
	{
		pg = alloc_pages(GFP_KERNEL|__GFP_ZERO|__GFP_HIGHMEM, DEV_DATSEG_ORDER);
		if(!pg)
		{
			ret = -ENOMEM;
			goto err_alloc;
		}
		pg->index = i;
		ret = radix_tree_insert(&simp_blkdev_data, i, pg);
		if(IS_ERR_VALUE(ret))
			goto err_insert_radix_tree;
	}
	return 0;
err_insert_radix_tree:
	__free_pages(pg, DEV_DATSEG_ORDER);
err_alloc:
	free_disk_mem();
	return ret;
}

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

struct block_device_operations simp_blkdev_fops = {
	.owner = THIS_MODULE,
};

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
		/* kmap一次只能映射一个高端内存页，vmap虽然可以映射多个，但太耗时。
		 */
		disk_mem = kmap(current_pg);
		/* page_address返回struct page对应的内存地址。
	 	* 如果struct page对应的是高端内存：
	 	*  若该高端内存页已被映射到内核地址空间，返回内核空间中的地址。
	 	*  若没有映射，返回NULL。这里使用需修改代码。
	 	disk_mem = page_address(start_page);*/
		if(!disk_mem)
		{
			printk("blkdev: get page's addr failed: %p\n", start_page);
			return -ENOMEM;
		}
		disk_mem += current_off;
		
		if(dir==0) /*READ*/
			memcpy(buf+count_done, disk_mem, current_cnt);
		else /*WRITE*/
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
		
		start_page = radix_tree_lookup(&simp_blkdev_data, ds_index);
		if(!start_page)
		{
			printk("blkdev: search mem failed: %lu\n", ds_index);
			return -ENOENT;
		}
		
		if(IS_ERR_VALUE(simp_blkdev_trans_oneseg(start_page, ds_offset,
						buf+count_done, count_current, dir)))
			return -EIO;
		
		count_done += count_current;
	}
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
