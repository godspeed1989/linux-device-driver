#ifndef IOMAP_H
#define IOMAP_H

/* the device structure */
struct Iomap
{
	unsigned long base;  /*mapped io base addr*/
	unsigned long size;  /*io addr size*/
	char *ptr;  /*new io base addr*/
};

#define IOMAP_GET	_IOR(0xbb, 0, struct Iomap)
#define IOMAP_SET	_IOW(0xbb, 1, struct Iomap)
#define IOMAP_CLEAR	_IOW(0xbb, 2, long)

#endif
