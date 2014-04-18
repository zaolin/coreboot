/*
 * Copyright (C) 2007 Juergen Beisert <juergen@kreuzholzen.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 *
 * Purpose:
 * Activate the VGA feature in a Geode GX1 based system with one of five
 * possible VESA modes: VGA, SVGA, XGA, 4:3 SXGA and 5:4 SXGA. Also it is
 * prepared to display a splash screen.
 */
#include <arch/io.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#include <device/pci_ids.h>
#include <console/console.h>
#include <cpu/amd/gx1def.h>
#include <delay.h>

#if CONFIG_GX1_VIDEO == 1
/*
 * Some register descriptions that are no listed in cpu/amd/gx1def.h
 */
#define CS5530_DOT_CLK_CONFIG	0x0024
#define CS5530_DISPLAY_CONFIG	0x0004

#define DC_FB_ST_OFFSET		0x8310	/* framebuffer start offset */
#define DC_CB_ST_OFFSET		0x8314	/* compression start offset */
#define DC_CURS_ST_OFFSET	0x8318	/* cursor start offset */
#define DC_VID_ST_OFFSET	0x8320	/* video start offset */
#define DC_LINE_DELTA		0x8324	/* fb and cb skip counts */
#define DC_BUF_SIZE		0x8328	/* fb and cb line size */
#define DC_H_TIMING_1		0x8330	/* horizontal timing... */
#define DC_H_TIMING_2		0x8334
#define DC_H_TIMING_3		0x8338
#define DC_FP_H_TIMING		0x833C
#define DC_V_TIMING_1		0x8340	/* vertical timing... */
#define DC_V_TIMING_2		0x8344
#define DC_V_TIMING_3		0x8348
#define DC_FP_V_TIMING		0x834C
#define DC_TIMING_CFG		0x8308
#define DC_OUTPUT_CFG		0x830C

/*
 * what colour depth should be used as default (in bpp)
 * Note: Currently no other value than 16 is supported
 */
#define COLOUR_DEPTH 16

/*
 * Support for a few basic video modes
 * Note: all modes only for CRT. The flatpanel feature is
 * not supported here (due to the lack of hardware to test)
 */
struct video_mode {
	int pixel_clock;		/*<< pixel clock in Hz */
	unsigned long pll_value;	/*<< pll register value for this clock */

	int visible_pixel;
	int hsync_start;
	int hsync_end;
	int line_length;

	int visible_lines;
	int vsync_start;
	int vsync_end;
	int picture_length;

	int sync_pol;		/*<< 0: low, 1: high, bit 0 hsync, bit 1 vsync */
};

/*
 * values for .sync_pol
 */
#define HSYNC_HIGH_POL 0
#define HSYNC_LOW_POL 1
#define VSYNC_HIGH_POL 0
#define VSYNC_LOW_POL 2

/* ModeLine "640x480" 31.5 640 664 704 832 480 489 491 520 -hsync -vsync */
static const struct video_mode mode_640x480 = {
	/*
	 * 640x480 @ 72Hz hsync: 37.9kHz
	 * VESA standard mode for classic 4:3 monitors
	 */
	.pixel_clock = 31500000,
	.pll_value = 0x33915801,

	.visible_pixel = 640,
	.hsync_start = 664,
	.hsync_end = 704,	/* 1.27 us sync length */
	.line_length = 832,	/* 26.39us */

	.visible_lines = 480,
	.vsync_start = 489,
	.vsync_end = 491,
	.picture_length = 520, /* 13.89ms */

	.sync_pol = HSYNC_LOW_POL | VSYNC_LOW_POL
};

/* ModeLine "800x600" 50.0 800 856 976 1040 600 637 643 666 +hsync +vsync */
static const struct video_mode mode_800x600 = {
	/*
	 * 800x600 @ 72Hz hsync: 48.1kHz
	 * VESA standard mode for classic 4:3 monitors
	 */
	.pixel_clock = 50000000,
	.pll_value = 0x23088801,

	.visible_pixel = 800,
	.hsync_start = 856,
	.hsync_end = 976,
	.line_length = 1040, /* 20.8us */

	.visible_lines = 600,
	.vsync_start = 637,
	.vsync_end = 643,
	.picture_length = 666, /* 13.89ms */

	.sync_pol = HSYNC_HIGH_POL | VSYNC_HIGH_POL
};

/* ModeLine "1024x768" 75.0 1024 1048 1184 1328 768 771 777 806 -hsync -vsync */
static const struct video_mode mode_1024x768 = {
	/*
	 * 1024x768 @ 70Hz (VESA) hsync: 56.5kHz
	 * Standard mode for classic 4:3 monitors
	 */
	.pixel_clock = 75000000,
	.pll_value = 0x37E22801,

	.visible_pixel = 1024,
	.hsync_start = 1048,
	.hsync_end = 1184,
	.line_length = 1328,	/* 17.7us */

	.visible_lines = 768,
	.vsync_start = 771,
	.vsync_end = 777,
	.picture_length = 806,	/* 14.3us */

	.sync_pol = HSYNC_LOW_POL | VSYNC_LOW_POL
};

/* ModeLine "1280x960" 108.0 1280 1376 1488 1800 960 961 964 1000 +hsync +vsync */
static const struct video_mode mode_1280x960 = {
	/*
	 * 1280x960 @ 60Hz (VESA) hsync: 60.0kHz
	 * Mode for classic 4:3 monitors
	 */
	.pixel_clock = 108000000,
	.pll_value = 0x2710C805,

	.visible_pixel = 1280,
	.hsync_start = 1376,
	.hsync_end = 1488,
	.line_length = 1800,	/* 16.67us */

	.visible_lines = 960,
	.vsync_start = 961,
	.vsync_end = 964,
	.picture_length = 1000,	/* 16.67ms */

	.sync_pol = HSYNC_HIGH_POL | VSYNC_HIGH_POL
};

/* ModeLine "1280x1024" 108.0 1280 1328 1440 1688 1024 1025 1028 1066 +hsync +vsync */
static const struct video_mode mode_1280x1024 = {
	/*
	 * 1280x1024 @ 60Hz (VESA) hsync: 64.0kHz
	 * Mode for modern 5:4 flat screens
	 */
	.pixel_clock = 108000000,
	.pll_value = 0x2710C805,

	.visible_pixel = 1280,
	.hsync_start = 1328,
	.hsync_end = 1440,
	.line_length = 1688,	/* 15.6us */

	.visible_lines = 1024,
	.vsync_start = 1025,
	.vsync_end = 1028,
	.picture_length = 1066,

	.sync_pol = HSYNC_HIGH_POL | VSYNC_HIGH_POL
};

/*
 * a few supported common modes
 */
static const struct video_mode *modes[] = {
	&mode_640x480,	/* CONFIG_GX1_VIDEOMODE = 0 */
	&mode_800x600,	/* CONFIG_GX1_VIDEOMODE = 1 */
	&mode_1024x768,	/* CONFIG_GX1_VIDEOMODE = 2 */
	&mode_1280x960,	/* CONFIG_GX1_VIDEOMODE = 3 */
	&mode_1280x1024	/* CONFIG_GX1_VIDEOMODE = 4 */
};

/* make a sanity check at buildtime */
#if CONFIG_GX1_VIDEOMODE > 4
# error Requested video mode is unknown!
#endif

/*
 * Setup the pixel PLL in the companion chip
 * base: register's base address
 * pll_val: pll register value to be set
 */
static void cs5530_set_clock_frequency(void *io_base,unsigned long pll_val)
{
	unsigned long reg;

	/* disable the PLL first, reset and power it down */
	reg = readl(io_base+CS5530_DOT_CLK_CONFIG) & ~0x20;
	reg |= 0x80000100;
	writel(reg, io_base+CS5530_DOT_CLK_CONFIG);

	/* write the new PLL setting */
	reg |= (pll_val & ~0x80000920);
	writel(reg, io_base+CS5530_DOT_CLK_CONFIG);

	mdelay(1);	/* wait for control voltage to be 0V */

	/* enable the PLL */
	reg |= 0x00000800;
	writel(reg, io_base+CS5530_DOT_CLK_CONFIG);

	/* clear reset */
	reg &= ~0x80000000;
	writel(reg, io_base+CS5530_DOT_CLK_CONFIG);

	/* clear bypass */
	reg &= ~0x00000100;
	writel(reg, io_base+CS5530_DOT_CLK_CONFIG);
}

/*
 * Setup memory layout
 * gx_base: GX register area
 * mode: Data about the video mode to setup
 *
 * This routine assumes unlocked DC registers. Using compressed buffer
 * is not supported! (makes more sense later, but not while booting)
 */
static void dc_setup_layout(void *gx_base,const struct video_mode *mode)
{
	unsigned long base = 0x00000000;

	writel(base, gx_base + DC_FB_ST_OFFSET);

	base += (COLOUR_DEPTH>>3) * mode->visible_pixel * mode->visible_lines;

	writel(base, gx_base + DC_CB_ST_OFFSET);
	writel(base, gx_base + DC_CURS_ST_OFFSET);
	writel(base, gx_base + DC_VID_ST_OFFSET);
	writel(((COLOUR_DEPTH>>3) * mode->visible_pixel) >> 2, gx_base + DC_LINE_DELTA);
	writel(((COLOUR_DEPTH>>3) * mode->visible_pixel) >> 3, gx_base + DC_BUF_SIZE);
}

/*
 * Setup the HSYNC/VSYNC, active video timing
 * gx_base: GX register area
 * mode: Data about the video mode to setup
 *
 * This routine assumes unlocked DC registers
 *
 * |<------------------------- htotal ----------------------------->|
 * |<------------ hactive -------------->|                          |
 * |                       hblankstart-->|                          |
 * |                                                    hblankend-->|
 * |                                hsyncstart-->|                  |
 * |                                           hsyncend-->|         |
 * |#####################################___________________________| RGB data
 * |______________________________________________---------_________| HSYNC
 *
 * |<------------------------- vtotal ----------------------------->|
 * |<------------ vactive -------------->|                          |
 * |                       vblankstart-->|                          |
 * |                                                    vblankend-->|
 * |                                vsyncstart-->|                  |
 * |                                           vsyncend-->|         |
 * |#####################################___________________________| line data
 * |______________________________________________---------_________| YSYNC
 */
static void dc_setup_timing(void *gx_base,const struct video_mode *mode)
{
	unsigned long hactive, hblankstart, hsyncstart, hsyncend, hblankend, htotal;
	unsigned long vactive, vblankstart, vsyncstart, vsyncend, vblankend, vtotal;

	hactive = mode->visible_pixel & 0x7FF;
	hblankstart = hactive;
	hsyncstart = mode->hsync_start & 0x7FF;
	hsyncend =  mode->hsync_end & 0x7FF;
	hblankend = mode->line_length & 0x7FF;
	htotal = hblankend;

	vactive = mode->visible_lines & 0x7FF;
	vblankstart = vactive;
	vsyncstart = mode->vsync_start & 0x7FF;
	vsyncend =  mode->vsync_end & 0x7FF;
	vblankend = mode->picture_length & 0x7FF;
	vtotal = vblankend;

	/* row description */
	writel((hactive - 1) | ((htotal - 1) << 16), gx_base + DC_H_TIMING_1);
	/* horizontal blank description */
	writel((hblankstart - 1) | ((hblankend - 1) << 16), gx_base + DC_H_TIMING_2);
	/* horizontal sync description */
	writel((hsyncstart - 1) | ((hsyncend - 1) << 16), gx_base + DC_H_TIMING_3);
	writel((hsyncstart - 1) | ((hsyncend - 1) << 16), gx_base + DC_FP_H_TIMING);

	/* line description */
	writel((vactive - 1) | ((vtotal - 1) << 16), gx_base + DC_V_TIMING_1);
	/* vertical blank description */
	writel((vblankstart - 1) | ((vblankend - 1) << 16), gx_base + DC_V_TIMING_2);
	/* vertical sync description */
	writel((vsyncstart - 1) | ((vsyncend - 1) << 16), gx_base + DC_V_TIMING_3);
	writel((vsyncstart - 2) | ((vsyncend - 2) << 16), gx_base + DC_FP_V_TIMING);
}

/*
 * Setup required internals to bring the mode up and running
 * gx_base: GX register area
 * mode: Data about the video mode to setup
 */
static void cs5530_activate_mode(void *gx_base, const struct video_mode *mode)
{
	writel(0x00000080, gx_base + DC_GENERAL_CFG);
	mdelay(1);
	dc_setup_layout(gx_base,mode);
	dc_setup_timing(gx_base,mode);

	writel(0x2000C581, gx_base + DC_GENERAL_CFG);
	writel(0x0000002F, gx_base + DC_TIMING_CFG);
	writel(0x00003004, gx_base + DC_OUTPUT_CFG);
}

/*
 * Activate the current mode to be "visible" outside
 * gx_base: GX register area
 * mode: Data about the video mode to setup
 */
static void cs5530_activate_video(void *io_base, const struct video_mode *mode)
{
	u32 val;

	val = mode->sync_pol;
	val <<= 8;

	writel(val | 0x0020002F, io_base + CS5530_DISPLAY_CONFIG);
}

/*
 * This bitmap file must provide:
 * int width: pixel count in one line
 * int height: line count
 * int colours: ount of used colour
 * unsigned long colour_map[]: RGB 565 colours to be used
 * unsigned char bitmap[]: index per pixel into colour_map[], width*height pixels
 */
#include "bitmap.c"

/*
 * show a boot splash screen in the right lower corner of the screen
 * swidth: screen width in pixel
 * sheight: screen height in lines
 * pitch: line pitch in bytes
 * base: screen base address
 *
 * This routine assumes we are using a 16 bit colour depth!
 */
static void show_boot_splash_16(u32 swidth,u32 sheight,u32 pitch,void *base)
{
	int word_count,i;
	unsigned short *adr;
	u32 xstart,ystart,x,y;
	/*
	 * fill the screen with the colour of the
	 * left top pixel in the graphic
	 */
	word_count = pitch*sheight;
	printk_debug("Clear Screen at %p, %d words\n",base,word_count);
	adr = (unsigned short *) base;
	for (i=0; i < word_count; i++, adr++)
		*adr = colour_map[bitmap[0]];
	printk_debug("Ready\n");

	/*
	 * paint the splash
	 */
	xstart=swidth-width;
	ystart=sheight-height;
	printk_debug("Start at %u,%u\n",xstart,ystart);
	for (y=0;y<height;y++) {
		adr=(unsigned short*)(base + pitch*(y+ystart)+2*xstart);
		for (x=0;x<width;x++) {
			*adr=(unsigned short)colour_map[(int)bitmap[x+y*width]];
			adr++;
		}
	}
}

/*
 * management part
 */
static void cs5530_vga_init(device_t dev)
{
	const struct video_mode *mode;
	void *io_base, *gx_base;

	io_base = (void*)pci_read_config32(dev,0x10);
	gx_base = (void*)GX_BASE;
	mode = modes[CONFIG_GX1_VIDEOMODE];

	printk_debug("Setting up video mode %dx%d with %d Hz clock\n",
		mode->visible_pixel, mode->visible_lines, mode->pixel_clock);

	cs5530_set_clock_frequency(io_base,mode->pll_value);

	writel(DC_UNLOCK_MAGIC, gx_base + DC_UNLOCK);

	show_boot_splash_16(mode->visible_pixel, mode->visible_lines,
		mode->visible_pixel*(COLOUR_DEPTH>>3),(void*)(GX_BASE+0x800000));

	cs5530_activate_mode(gx_base,mode);

	cs5530_activate_video(io_base, mode);
	writel(0x00000000, gx_base + DC_UNLOCK);
}

static struct device_operations vga_ops = {
	.read_resources   = pci_dev_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init             = cs5530_vga_init,
	.enable           = NULL /* not required */
};

static struct pci_driver vga_pci_driver __pci_driver = {
	.ops 	= &vga_ops,
	.vendor = PCI_VENDOR_ID_CYRIX,
	.device = PCI_DEVICE_ID_CYRIX_5530_VIDEO
};

#endif /* #if CONFIG_GX1_VIDEO == 1 */
