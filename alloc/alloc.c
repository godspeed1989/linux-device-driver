#include <asm/page.h>
#include <asm/io.h>
#include <linux/module.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#define UINT unsigned int
#define MB(addr) (((UINT)addr)>>20)
#define page_to_virt(page)	\
	((((page) - mem_map) << PAGE_SHIFT) + PAGE_OFFSET)

void * free_pg;
struct page * alloc_pg;
void * kmallocmem;
void * vmallocmem;

int __init mem_module_init(void)
{
	UINT phys, virt;
	printk("-----------------\n");
	printk("PAGE_OFFSET = %x\n", (UINT)PAGE_OFFSET);
	
	printk("This module addr: %x(%u MB)\n", (UINT)&virt, MB(&virt));
	phys = virt_to_phys((void*)&virt);
	printk("phys: %x(%u MB)\n", phys, MB(phys));
	
	
	/*申请的内存位于物理内存映射区域，与真实物理地址只有一个固定偏移*/
	free_pg = (void*)__get_free_page(0);
	virt = (UINT)free_pg;
	printk("->Free page mem: %x(%u MB)\n", virt, MB(virt));
	phys = virt_to_phys(free_pg);
	printk("phys: %x(%u MB)\n", phys, MB(phys));
	
	alloc_pg = alloc_page(GFP_KERNEL);
	virt = page_to_virt(alloc_pg);
	printk("->Alloc page mem: %x(%u MB)\n", virt, MB(virt));
	phys = virt_to_phys(alloc_pg);
	printk("phys: %x(%u MB)\n", phys, MB(phys));

	/*申请的内存位于物理内存映射区域，与真实物理地址只有一个固定偏移*/
	kmallocmem = (void*)kmalloc(100, 0);
	virt = (UINT)kmallocmem;
	printk("->Kmalloc mem: %x(%u MB)\n", virt, MB(virt));
	phys = virt_to_phys(kmallocmem);
	printk("phys: %x(%u MB)\n", phys, MB(phys));

	/*申请的内存位于vmalloc_start~vmalloc_end之间，与物理地址没有简短的转换关系*/
	vmallocmem = (void*)vmalloc(100000);
	virt = (UINT)vmallocmem;
	printk("->Vmalloc mem: %x(%u MB)\n", virt, MB(virt));
	phys = virt_to_phys(vmallocmem);
	printk("phys: %x(%u MB)\n", phys, MB(phys));
	
	printk("-----------------\n");
	return 0;
}
module_init(mem_module_init);

void __exit mem_module_exit(void)
{
	free_page((long unsigned int)free_pg);
	__free_page(alloc_pg);
	kfree(kmallocmem);
	vfree(vmallocmem);
}
module_exit(mem_module_exit);
MODULE_LICENSE("GPL");
