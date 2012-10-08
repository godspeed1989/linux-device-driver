#include <linux/module.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/serial.h>
#include <linux/serial_core.h>

#define  DELAY_TIME  (HZ*2)
struct timer_list * timer;

void tiny_set_termios(struct uart_port *port,
		struct ktermios *new, struct ktermios *old)
{
	
}

void tiny_stop_tx(struct uart_port *port)
{
}

void __tiny_tx_chars(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;
	int count;

	if (port->x_char) {
		pr_debug("wrote %2x", port->x_char);
		port->icount.tx++;
		port->x_char = 0;
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		tiny_stop_tx(port);
		return;
	}

	count = port->fifosize >> 1;
	do {
		pr_debug("wrote %2x", xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (uart_circ_empty(xmit))
		tiny_stop_tx(port);
}

void _tiny_timer(unsigned long data)
{
	struct uart_port *port = (struct uart_port*)data;
	struct tty_struct *tty;
	if(!port || !port->state)
		return;
	tty = port->state->port.tty;
	if(!tty)
		return;
	
	tty_insert_flip_char(tty, 't', 0);
	tty_flip_buffer_push(tty);

	timer->expires = jiffies + DELAY_TIME;
	add_timer(timer);

	__tiny_tx_chars(port);
}

int tiny_startup(struct uart_port *uart_port)
{
	if(timer == NULL) {
		timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
		if(timer == NULL)	return -ENOMEM;
	}
	timer->data = (unsigned long)uart_port;
	timer->expires = jiffies + DELAY_TIME;
	timer->function = _tiny_timer;
	add_timer(timer);
	return 0;
}

void tiny_shutdown(struct uart_port *port)
{
	if(timer)	del_timer(timer);
}

struct uart_ops tiny_serial_ops = {
	.startup = tiny_startup,
	.shutdown = tiny_shutdown,
	.set_termios = tiny_set_termios,
	.stop_tx = tiny_stop_tx,
};

struct uart_port tiny_serial_port = {
	.ops = &tiny_serial_ops,
};

struct uart_driver tiny_serial_drv = {
	.driver_name = "tty_tiny",
	.dev_name = "tty_tiny",
	.major = 240,
	.minor = 1,
	.nr = 1,
};

int __init tiny_serial_init(void)
{
	int ret;
	timer = NULL;
	
	ret = uart_register_driver(&tiny_serial_drv);
	if(ret)
		return ret;
	
	ret = uart_add_one_port(&tiny_serial_drv, &tiny_serial_port);
	if(ret)
		return ret;
		
	return ret;
}
module_init(tiny_serial_init);

void __exit tiny_serial_exit(void)
{
	
}
module_exit(tiny_serial_exit);
MODULE_LICENSE("GPL");
