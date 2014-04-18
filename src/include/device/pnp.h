#ifndef DEVICE_PNP_H
#define DEVICE_PNP_H

static inline void pnp_write_config(unsigned char port, unsigned char value, unsigned char reg)
{
	outb(reg, port);
	outb(value, port +1);
}

static inline unsigned char pnp_read_config(unsigned char port, unsigned char reg)
{
	outb(reg, port);
	return inb(port +1);
}

static inline void pnp_set_logical_device(unsigned char port, int device)
{
	pnp_write_config(port, device, 0x07);
}

static inline void pnp_set_enable(unsigned char port, int enable)
{
	pnp_write_config(port, enable?0x1:0x0, 0x30);
}

static inline int pnp_read_enable(unsigned char port)
{
	return !!pnp_read_config(port, 0x30);
}

static inline void pnp_set_iobase0(unsigned char port, unsigned iobase)
{
	pnp_write_config(port, (iobase >> 8) & 0xff, 0x60);
	pnp_write_config(port, iobase & 0xff, 0x61);
}

static inline void pnp_set_iobase1(unsigned char port, unsigned iobase)
{
	pnp_write_config(port, (iobase >> 8) & 0xff, 0x62);
	pnp_write_config(port, iobase & 0xff, 0x63);
}

static inline void pnp_set_irq0(unsigned char port, unsigned irq)
{
	pnp_write_config(port, irq, 0x70);
}

static inline void pnp_set_irq1(unsigned char port, unsigned irq)
{
	pnp_write_config(port, irq, 0x72);
}

static inline void pnp_set_drq(unsigned char port, unsigned drq)
{
	pnp_write_config(port, drq & 0xff, 0x74);
}

#endif /* DEVICE_PNP_H */
