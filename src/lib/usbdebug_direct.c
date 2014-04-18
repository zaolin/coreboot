/*
 * Copyright (C) 2006 Eric Biederman (ebiederm@xmission.com)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License version
 *	2 as published by the Free Software Foundation.
 *
 *	2006.12.10 yhlu moved it to corbeoot and use struct instead
 */
#ifndef __ROMCC__
#include <console/console.h>
#else
#if CONFIG_USE_PRINTK_IN_CAR==0
#define printk_debug(fmt, arg...)   do {} while(0)
#endif
#endif


#include <arch/io.h>

#include <usb_ch9.h>
#include <ehci.h>
#include <usbdebug_direct.h>

#define USB_DEBUG_DEVNUM 127

#define DBGP_DATA_TOGGLE	0x8800
#define DBGP_PID_UPDATE(x, tok) \
	((((x) ^ DBGP_DATA_TOGGLE) & 0xffff00) | ((tok) & 0xff))

#define DBGP_LEN_UPDATE(x, len) (((x) & ~0x0f) | ((len) & 0x0f))
/*
 * USB Packet IDs (PIDs)
 */

/* token */
#define USB_PID_OUT		0xe1
#define USB_PID_IN		0x69
#define USB_PID_SOF		0xa5
#define USB_PID_SETUP		0x2d
/* handshake */
#define USB_PID_ACK		0xd2
#define USB_PID_NAK		0x5a
#define USB_PID_STALL		0x1e
#define USB_PID_NYET		0x96
/* data */
#define USB_PID_DATA0		0xc3
#define USB_PID_DATA1		0x4b
#define USB_PID_DATA2		0x87
#define USB_PID_MDATA		0x0f
/* Special */
#define USB_PID_PREAMBLE	0x3c
#define USB_PID_ERR		0x3c
#define USB_PID_SPLIT		0x78
#define USB_PID_PING		0xb4
#define USB_PID_UNDEF_0		0xf0

#define USB_PID_DATA_TOGGLE	0x88
#define DBGP_CLAIM (DBGP_OWNER | DBGP_ENABLED | DBGP_INUSE)

#define PCI_CAP_ID_EHCI_DEBUG	0xa

#define HUB_ROOT_RESET_TIME	50	/* times are in msec */
#define HUB_SHORT_RESET_TIME	10
#define HUB_LONG_RESET_TIME	200
#define HUB_RESET_TIMEOUT	500

#define DBGP_MAX_PACKET		8

static int dbgp_wait_until_complete(struct ehci_dbg_port *ehci_debug)
{
	unsigned ctrl;
	int loop = 0x100000;
	do {
		ctrl = readl(&ehci_debug->control);
		/* Stop when the transaction is finished */
		if (ctrl & DBGP_DONE)
			break;
	} while(--loop>0); 

	if (!loop) return -1000;

	/* Now that we have observed the completed transaction,
	 * clear the done bit.
	 */
	writel(ctrl | DBGP_DONE, &ehci_debug->control);
	return (ctrl & DBGP_ERROR) ? -DBGP_ERRCODE(ctrl) : DBGP_LEN(ctrl);
}

static void dbgp_mdelay(int ms)
{
	int i;
	while (ms--) {
		for (i = 0; i < 1000; i++)
			inb(0x80);
	}
}

static void dbgp_breath(void)
{
	/* Sleep to give the debug port a chance to breathe */
}

static int dbgp_wait_until_done(struct ehci_dbg_port *ehci_debug, unsigned ctrl)
{
	unsigned pids, lpid;
	int ret;

	int loop = 3;
retry:
	writel(ctrl | DBGP_GO, &ehci_debug->control);
	ret = dbgp_wait_until_complete(ehci_debug);
	pids = readl(&ehci_debug->pids);
	lpid = DBGP_PID_GET(pids);

	if (ret < 0)
		return ret;

	/* If the port is getting full or it has dropped data
	 * start pacing ourselves, not necessary but it's friendly.
	 */
	if ((lpid == USB_PID_NAK) || (lpid == USB_PID_NYET))
		dbgp_breath();
	
	/* If I get a NACK reissue the transmission */
	if (lpid == USB_PID_NAK) {
		if (--loop > 0) goto retry;
	}

	return ret;
}

static void dbgp_set_data(struct ehci_dbg_port *ehci_debug, const void *buf, int size)
{
	const unsigned char *bytes = buf;
	unsigned lo, hi;
	int i;
	lo = hi = 0;
	for (i = 0; i < 4 && i < size; i++)
		lo |= bytes[i] << (8*i);
	for (; i < 8 && i < size; i++)
		hi |= bytes[i] << (8*(i - 4));
	writel(lo, &ehci_debug->data03);
	writel(hi, &ehci_debug->data47);
}

static void dbgp_get_data(struct ehci_dbg_port *ehci_debug, void *buf, int size)
{
	unsigned char *bytes = buf;
	unsigned lo, hi;
	int i;
	lo = readl(&ehci_debug->data03);
	hi = readl(&ehci_debug->data47);
	for (i = 0; i < 4 && i < size; i++)
		bytes[i] = (lo >> (8*i)) & 0xff;
	for (; i < 8 && i < size; i++)
		bytes[i] = (hi >> (8*(i - 4))) & 0xff;
}

static int dbgp_bulk_write(struct ehci_dbg_port *ehci_debug, unsigned devnum, unsigned endpoint, const char *bytes, int size)
{
	unsigned pids, addr, ctrl;
	int ret;
	if (size > DBGP_MAX_PACKET)
		return -1;

	addr = DBGP_EPADDR(devnum, endpoint);

	pids = readl(&ehci_debug->pids);
	pids = DBGP_PID_UPDATE(pids, USB_PID_OUT);
	
	ctrl = readl(&ehci_debug->control);
	ctrl = DBGP_LEN_UPDATE(ctrl, size);
	ctrl |= DBGP_OUT;
	ctrl |= DBGP_GO;

	dbgp_set_data(ehci_debug, bytes, size);
	writel(addr, &ehci_debug->address);
	writel(pids, &ehci_debug->pids);

	ret = dbgp_wait_until_done(ehci_debug, ctrl);
	if (ret < 0) {
		return ret;
	}
	return ret;
}

int dbgp_bulk_write_x(struct ehci_debug_info *dbg_info, const char *bytes, int size)
{
	return dbgp_bulk_write(dbg_info->ehci_debug, dbg_info->devnum, dbg_info->endpoint_out, bytes, size);
}

static int dbgp_bulk_read(struct ehci_dbg_port *ehci_debug, unsigned devnum, unsigned endpoint, void *data, int size)
{
	unsigned pids, addr, ctrl;
	int ret;

	if (size > DBGP_MAX_PACKET)
		return -1;

	addr = DBGP_EPADDR(devnum, endpoint);

	pids = readl(&ehci_debug->pids);
	pids = DBGP_PID_UPDATE(pids, USB_PID_IN);
		
	ctrl = readl(&ehci_debug->control);
	ctrl = DBGP_LEN_UPDATE(ctrl, size);
	ctrl &= ~DBGP_OUT;
	ctrl |= DBGP_GO;
		
	writel(addr, &ehci_debug->address);
	writel(pids, &ehci_debug->pids);
	ret = dbgp_wait_until_done(ehci_debug, ctrl);
	if (ret < 0)
		return ret;
	if (size > ret)
		size = ret;
	dbgp_get_data(ehci_debug, data, size);
	return ret;
}
int dbgp_bulk_read_x(struct ehci_debug_info *dbg_info, void *data, int size)
{
	return dbgp_bulk_read(dbg_info->ehci_debug, dbg_info->devnum, dbg_info->endpoint_in, data, size);
}

static int dbgp_control_msg(struct ehci_dbg_port *ehci_debug, unsigned devnum, int requesttype, int request, 
	int value, int index, void *data, int size)
{
	unsigned pids, addr, ctrl;
	struct usb_ctrlrequest req;
	int read;
	int ret;

	read = (requesttype & USB_DIR_IN) != 0;
	if (size > (read?DBGP_MAX_PACKET:0))
		return -1;
	
	/* Compute the control message */
	req.bRequestType = requesttype;
	req.bRequest = request;
	req.wValue = value;
	req.wIndex = index;
	req.wLength = size;

	pids = DBGP_PID_SET(USB_PID_DATA0, USB_PID_SETUP);
	addr = DBGP_EPADDR(devnum, 0);

	ctrl = readl(&ehci_debug->control);
	ctrl = DBGP_LEN_UPDATE(ctrl, sizeof(req));
	ctrl |= DBGP_OUT;
	ctrl |= DBGP_GO;

	/* Send the setup message */
	dbgp_set_data(ehci_debug, &req, sizeof(req));
	writel(addr, &ehci_debug->address);
	writel(pids, &ehci_debug->pids);
	ret = dbgp_wait_until_done(ehci_debug, ctrl);
	if (ret < 0)
		return ret;


	/* Read the result */
	ret = dbgp_bulk_read(ehci_debug, devnum, 0, data, size);
	return ret;
}

static int ehci_reset_port(struct ehci_regs *ehci_regs, int port)
{
	unsigned portsc;
	unsigned delay_time, delay;
	int loop;

	/* Reset the usb debug port */
	portsc = readl(&ehci_regs->port_status[port - 1]);
	portsc &= ~PORT_PE;
	portsc |= PORT_RESET;
	writel(portsc, &ehci_regs->port_status[port - 1]);

	delay = HUB_ROOT_RESET_TIME;
	for (delay_time = 0; delay_time < HUB_RESET_TIMEOUT;
	     delay_time += delay) {
		dbgp_mdelay(delay);

		portsc = readl(&ehci_regs->port_status[port - 1]);
		if (portsc & PORT_RESET) {
			/* force reset to complete */
			loop = 2;
			writel(portsc & ~(PORT_RWC_BITS | PORT_RESET), 
				&ehci_regs->port_status[port - 1]);
			do {	
				dbgp_mdelay(delay);
				portsc = readl(&ehci_regs->port_status[port - 1]);
				delay_time += delay;
			} while ((portsc & PORT_RESET) && (--loop > 0));
			if (!loop) {
				printk_debug("ehci_reset_port forced done");
			}
		}

		/* Device went away? */
		if (!(portsc & PORT_CONNECT))
			return -107;//-ENOTCONN;

		/* bomb out completely if something weird happend */
		if ((portsc & PORT_CSC))
			return -22;//-EINVAL;

		/* If we've finished resetting, then break out of the loop */
		if (!(portsc & PORT_RESET) && (portsc & PORT_PE))
			return 0;
	}
	return -16;//-EBUSY;
}

static int ehci_wait_for_port(struct ehci_regs *ehci_regs, int port)
{
	unsigned status;
	int ret, reps;
	for (reps = 0; reps < 3; reps++) {
		dbgp_mdelay(100);
		status = readl(&ehci_regs->status);
		if (status & STS_PCD) {
			ret = ehci_reset_port(ehci_regs, port);
			if (ret == 0)
				return 0;
		}
	}
	return -107; //-ENOTCONN;
}


#define DBGP_DEBUG 1
#if DBGP_DEBUG
# define dbgp_printk printk_debug
#else
#define dbgp_printk(fmt, arg...)   do {} while(0)
#endif
static void usbdebug_direct_init(unsigned ehci_bar, unsigned offset, struct ehci_debug_info *info)
{
	struct ehci_caps *ehci_caps;
	struct ehci_regs *ehci_regs;
	struct ehci_dbg_port *ehci_debug;
	unsigned dbgp_endpoint_out;
	unsigned dbgp_endpoint_in;
	struct usb_debug_descriptor dbgp_desc;
	unsigned ctrl, devnum;
	int ret;
	unsigned delay_time, delay;
	int loop;

	unsigned cmd,  status, portsc, hcs_params, debug_port, n_ports, new_debug_port;
	int i;
	unsigned port_map_tried;

	unsigned playtimes = 3;

	ehci_caps  = (struct ehci_caps *)ehci_bar;
	ehci_regs  = (struct ehci_regs *)(ehci_bar + HC_LENGTH(readl(&ehci_caps->hc_capbase)));
	ehci_debug = (struct ehci_dbg_port *)(ehci_bar + offset);

	info->ehci_debug = (void *)0;

try_next_time:
	port_map_tried = 0;

try_next_port:
	hcs_params = readl(&ehci_caps->hcs_params);
	debug_port = HCS_DEBUG_PORT(hcs_params);
	n_ports    = HCS_N_PORTS(hcs_params);

	dbgp_printk("ehci_bar: 0x%x\n", ehci_bar);
	dbgp_printk("debug_port: %d\n", debug_port);
	dbgp_printk("n_ports:    %d\n", n_ports);

#if 1
        for (i = 1; i <= n_ports; i++) {
                portsc = readl(&ehci_regs->port_status[i-1]);
                dbgp_printk("PORTSC #%d: %08x\n", i, portsc);
        }
#endif

	if(port_map_tried && (new_debug_port!=debug_port)) {
		if(--playtimes) {
			set_debug_port(debug_port);
			goto try_next_time;
		}
		return;	
	}

	/* Reset the EHCI controller */
	loop = 10;
	cmd = readl(&ehci_regs->command);
	cmd |= CMD_RESET;
	writel(cmd, &ehci_regs->command);
	do {
		cmd = readl(&ehci_regs->command);
	} while ((cmd & CMD_RESET) && (--loop > 0));

	if(!loop) {
		dbgp_printk("Could not reset EHCI controller.\n");
		return;
	}
	dbgp_printk("EHCI controller reset successfully.\n");

	/* Claim ownership, but do not enable yet */
	ctrl = readl(&ehci_debug->control);
	ctrl |= DBGP_OWNER;
	ctrl &= ~(DBGP_ENABLED | DBGP_INUSE);
	writel(ctrl, &ehci_debug->control);

	/* Start the ehci running */
	cmd = readl(&ehci_regs->command);
	cmd &= ~(CMD_LRESET | CMD_IAAD | CMD_PSE | CMD_ASE | CMD_RESET);
	cmd |= CMD_RUN;
	writel(cmd, &ehci_regs->command);

	/* Ensure everything is routed to the EHCI */
	writel(FLAG_CF, &ehci_regs->configured_flag);

	/* Wait until the controller is no longer halted */
	loop = 10;
	do {
		status = readl(&ehci_regs->status);
	} while ((status & STS_HALT) && (--loop>0));

	if(!loop) {
		dbgp_printk("EHCI could not be started.\n");
		return;
	}
	dbgp_printk("EHCI started.\n");

	/* Wait for a device to show up in the debug port */
	ret = ehci_wait_for_port(ehci_regs, debug_port);
	if (ret < 0) {
		dbgp_printk("No device found in debug port %d\n", debug_port);
		goto next_debug_port;
	}
	dbgp_printk("EHCI done waiting for port.\n");

	/* Enable the debug port */
	ctrl = readl(&ehci_debug->control);
	ctrl |= DBGP_CLAIM;
	writel(ctrl, &ehci_debug->control);
	ctrl = readl(&ehci_debug->control);
	if ((ctrl & DBGP_CLAIM) != DBGP_CLAIM) {
		dbgp_printk("No device in EHCI debug port.\n");
		writel(ctrl & ~DBGP_CLAIM, &ehci_debug->control);
		goto err;
	}
	dbgp_printk("EHCI debug port enabled.\n");

	/* Completely transfer the debug device to the debug controller */
	portsc = readl(&ehci_regs->port_status[debug_port - 1]);
	portsc &= ~PORT_PE;
	writel(portsc, &ehci_regs->port_status[debug_port - 1]);

	dbgp_mdelay(100);

	/* Find the debug device and make it device number 127 */
	for (devnum = 0; devnum <= 127; devnum++) {
		ret = dbgp_control_msg(ehci_debug, devnum,
			USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
			USB_REQ_GET_DESCRIPTOR, (USB_DT_DEBUG << 8), 0,
			&dbgp_desc, sizeof(dbgp_desc));
		if (ret > 0)
			break;
	}
	if (devnum > 127) {
		dbgp_printk("Could not find attached debug device.\n");
		goto err;
	}
	if (ret < 0) {
		dbgp_printk("Attached device is not a debug device.\n");
		goto err;
	}
	dbgp_endpoint_out = dbgp_desc.bDebugOutEndpoint;
	dbgp_endpoint_in = dbgp_desc.bDebugInEndpoint;

	/* Move the device to 127 if it isn't already there */
	if (devnum != USB_DEBUG_DEVNUM) {
		ret = dbgp_control_msg(ehci_debug, devnum,
			USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
			USB_REQ_SET_ADDRESS, USB_DEBUG_DEVNUM, 0, (void *)0, 0);
		if (ret < 0) {
			dbgp_printk("Could not move attached device to %d.\n", 
				USB_DEBUG_DEVNUM);
			goto err;
		}
		devnum = USB_DEBUG_DEVNUM;
		dbgp_printk("EHCI debug device renamed to 127.\n");
	}

	/* Enable the debug interface */
	ret = dbgp_control_msg(ehci_debug, USB_DEBUG_DEVNUM,
		USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
		USB_REQ_SET_FEATURE, USB_DEVICE_DEBUG_MODE, 0, (void *)0, 0);
	if (ret < 0) {
		dbgp_printk("Could not enable EHCI debug device.\n");
		goto err;
	}
	dbgp_printk("EHCI debug interface enabled.\n");

	/* Perform a small write to get the even/odd data state in sync
	 */
	ret = dbgp_bulk_write(ehci_debug, USB_DEBUG_DEVNUM, dbgp_endpoint_out, " ",1);
	if (ret < 0) {
		dbgp_printk("dbgp_bulk_write failed: %d\n", ret);
		goto err;
	}
	dbgp_printk("Test write done\n");

	info->ehci_caps = ehci_caps;
	info->ehci_regs = ehci_regs;
	info->ehci_debug = ehci_debug;
	info->devnum = devnum;
	info->endpoint_out = dbgp_endpoint_out;
	info->endpoint_in = dbgp_endpoint_in;
	
	return;
err:
	/* Things didn't work so remove my claim */
	ctrl = readl(&ehci_debug->control);
	ctrl &= ~(DBGP_CLAIM | DBGP_OUT);
	writel(ctrl, &ehci_debug->control);

next_debug_port:
	port_map_tried |= (1<<(debug_port-1));
	if(port_map_tried != ((1<<n_ports) -1)) {
		new_debug_port = ((debug_port-1+1)%n_ports) + 1;
		set_debug_port(new_debug_port);
		goto try_next_port;
	}
	if(--playtimes) {
		set_debug_port(debug_port);
		goto try_next_time;
	}

}


