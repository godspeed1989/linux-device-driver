#ifndef PROC_H
#define PROC_H


int proc_read_current(char *page, char **start, off_t off, int count,
                      int *eof, void *data);
int proc_read_hello(char *page, char **start, off_t off, int count,
                      int *eof, void *data);
int proc_write_hello(struct file *fiel, const char *buffer, 
                     unsigned long count,void *data);


#endif

