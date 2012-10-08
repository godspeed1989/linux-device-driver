#include <asm/io.h>
#include "cmos.h"

extern unsigned char addr_ports[NUM_CMOS_BANKS];
extern unsigned char data_ports[NUM_CMOS_BANKS];

unsigned int port_data_in(unsigned char offset, int bank)
{
	unsigned char data;
	if (unlikely(bank >= NUM_CMOS_BANKS)) {
		MSG("port_in: err bank\n"); return 0;
	} else {
		outb(offset, addr_ports[bank]);
		data = inb(data_ports[bank]);
	}
	return data;
}

void port_data_out(unsigned char offset, unsigned char data, int bank)
{
	if (unlikely(bank >= NUM_CMOS_BANKS)) {
		MSG("port_out: err bank\n");
	} else {
		outb(offset, addr_ports[bank]);
		outb(data, data_ports[bank]);
	}
}
