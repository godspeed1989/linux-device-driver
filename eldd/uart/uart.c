#include <linux/console.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <asm/irq.h>
#include <asm/io.h>

#define USB_UART_MAJOR              200
#define USB_UART_MINOR_START        70
#define USB_UART_PORTS              2
#define PORT_USB_UART               30

#define USB_UART1_BASE              0xe8000000
#define USB_UART2_BASE              0xe9000000
#define USB_UART_REGISTER_SPACE     0x3
/* semantics of bits in the status register */
#define USB_UART_TX_FULL            0x20
#define USB_UART_RX_EMPTY           0x10
#define USB_UART_STATUS             0x0F/*parity/frame/overrun*/

#define USB_UART1_IRQ               3
#define USB_UART2_IRQ               4
#define USB_UART_FIFO_SIZE          32
#define USB_UART_CLK_FREQ           16000000



struct uart_driver usb_uart_driver = 
{
	.owner			= THIS_MODULE,
	.driver_name	= "usb_uart",
	.dev_name		= "ttyUU",
	.major			= USB_UART_MAJOR,
	.minor			= USB_UART_MINOR_START,
	.nr				= USB_UART_PORTS,
};

void usb_uart_putc(struct uart_port *port, unsigned char c)
{
	while(__raw_readb(port->membase) & USB_UART_TX_FULL);
	
	__raw_writeb(c, (port->membase+1));
}
unsigned char usb_uart_getc(struct uart_port *port)
{
	while(__raw_readb(port->membase) & USB_UART_RX_EMPTY);
	
	return __raw_readb(port->membase+2);
}
void usb_uart_start_tx(struct uart_port *port)
{
	while(1)
	{
		usb_uart_putc(port, 
			port->state->xmit.buf[port->state->xmit.tail]);
		port->state->xmit.tail = 
			(port->state->xmit.tail+1) & (UART_XMIT_SIZE-1);
		port->icount.tx++;
		if(uart_circ_empty(&port->state->xmit)) break;
	}
}
unsigned char usb_uart_status(struct uart_port *port)
{
	return (__raw_readb(port->membase) & USB_UART_STATUS);
}
unsigned int more_chars_to_be_read(void)
{	return 0;	}
irqreturn_t usb_uart_rxint(int irq, void *dev_id)
{
	unsigned int data, status;
	struct uart_port *port = (struct uart_port*)dev_id;
	struct tty_struct *tty = port->state->port.tty;
	do {
		data = usb_uart_getc(port);
		status = usb_uart_status(port);
		tty_insert_flip_char(tty, data, status);
	} while(more_chars_to_be_read());
	return IRQ_HANDLED;
}
int usb_uart_request_port(struct uart_port *port)
{
	struct resource *retval;
	retval = request_mem_region(port->mapbase, 
				(u32)USB_UART_REGISTER_SPACE, "usb_uart");
	if(retval)
		return -EBUSY;
	return 0;
}
void usb_uart_release_port(struct uart_port *port)
{
	release_mem_region(port->mapbase, USB_UART_REGISTER_SPACE);
}
int usb_uart_startup(struct uart_port* port)
{
	int retval;
	retval = request_irq(port->irq, usb_uart_rxint, 0,
						 "usb_uart", (void*)port);
	return retval;
}
void usb_uart_shutdown(struct uart_port *port)
{
	free_irq(port->irq, port);
}
struct uart_ops usb_uart_ops = 
{
	.start_tx     = usb_uart_start_tx,
	.startup      = usb_uart_startup,
	.shutdown     = usb_uart_shutdown,
	.request_port = usb_uart_request_port,
	.release_port = usb_uart_release_port,
};

struct uart_port usb_uart_port[] = 
{
	{
		.ops     = &usb_uart_ops,
		.mapbase = (unsigned int) USB_UART1_BASE,
		.iotype  = UPIO_MEM,
		.irq     = USB_UART1_IRQ,
		.uartclk = USB_UART_CLK_FREQ,
		.fifosize = USB_UART_FIFO_SIZE,
		.flags   = UPF_BOOT_AUTOCONF,
		.line    = 0,
	},
	{
		.ops     = &usb_uart_ops,
		.mapbase = (unsigned int) USB_UART2_BASE,
		.iotype  = UPIO_MEM,
		.irq     = USB_UART2_IRQ,
		.uartclk = USB_UART_CLK_FREQ,
		.fifosize = USB_UART_FIFO_SIZE,
		.flags   = UPF_BOOT_AUTOCONF,
		.line    = 1,
	},
};
/************* platform driver *************/
int usb_uart_plat_suspend(struct platform_device *dev, pm_message_t s)
{
	uart_suspend_port(&usb_uart_driver, &usb_uart_port[dev->id]);
	return 0;
}
int usb_uart_plat_resume(struct platform_device *dev)
{
	uart_resume_port(&usb_uart_driver, &usb_uart_port[dev->id]);
	return 0;
}
int usb_uart_plat_probe(struct platform_device *dev)
{
	/* Use 'usb_uart_driver' to drive 'usb_uart_port[i]' */
	uart_add_one_port(&usb_uart_driver, &usb_uart_port[dev->id]);
	platform_set_drvdata(dev, &usb_uart_port[dev->id]);
	return 0;
}
int usb_uart_plat_remove(struct platform_device *dev)
{
	platform_set_drvdata(dev, NULL);
	/* remove port to uart driver */
	uart_remove_one_port(&usb_uart_driver, &usb_uart_port[dev->id]);
	return 0;
}
struct platform_driver usb_uart_plat_driver =
{
	.probe  = usb_uart_plat_probe,
	.remove = usb_uart_plat_remove,
	.resume = usb_uart_plat_resume,
	.suspend = usb_uart_plat_suspend,
	.driver = { .name = "usb_uart" },
};

struct platform_device *usb_uart_plat_device1;
struct platform_device *usb_uart_plat_device2;

/********* init and exit *********/
int __init usb_uart_init(void)
{
	int retval;
	/* register uart driver */
	retval = uart_register_driver(&usb_uart_driver);
	if(!retval) return retval;

	/* register platform device, 
	   usually called during arch-specific setup */
	usb_uart_plat_device1 = platform_device_register_simple
							("usb_uart", 0, NULL, 0);
	if(IS_ERR(usb_uart_plat_device1)) {
		uart_unregister_driver(&usb_uart_driver);
		return PTR_ERR(usb_uart_plat_device1);
	}
	usb_uart_plat_device2 = platform_device_register_simple
							("usb_uart", 1, NULL, 0);
	if(IS_ERR(usb_uart_plat_device2)) {
		uart_unregister_driver(&usb_uart_driver);
		platform_device_unregister(usb_uart_plat_device1);
		return PTR_ERR(usb_uart_plat_device2);
	}
	
	/* Announce a matching plat-driver for plat-dev registered above */
	retval = platform_driver_register(&usb_uart_plat_driver);
	if(retval) {
		uart_unregister_driver(&usb_uart_driver);
		platform_device_unregister(usb_uart_plat_device1);
		platform_device_unregister(usb_uart_plat_device2);
		return -EFAULT;
	}
	
	return 0;
}
module_init(usb_uart_init);

void __exit usb_uart_exit(void)
{
}
module_exit(usb_uart_exit);
