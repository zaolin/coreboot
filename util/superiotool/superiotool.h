/*
 * This file is part of the LinuxBIOS project.
 *
 * Copyright (C) 2007 Carl-Daniel Hailfinger
 * Copyright (C) 2007 Uwe Hermann <uwe@hermann-uwe.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef SUPERIOTOOL_H
#define SUPERIOTOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <sys/io.h>

#define SUPERIOTOOL_VERSION "0.1"

#define USAGE "Usage: superiotool [-d] [-V] [-v] [-h]\n\n\
  -d | --dump      Dump Super I/O registers\n\
  -V | --verbose   Verbose mode\n\
  -v | --version   Show the superiotool version\n\
  -h | --help      Show a short help text\n\n\
Per default (no options) superiotool will just probe for a Super I/O\n\
and print its vendor, name, ID, version, and config port.\n"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define EOT		-1		/* End Of Table */
#define NOLDN		-2		/* NO LDN needed */
#define NANA		-3		/* Not Available */
#define RSVD		-4		/* Reserved */
#define MISC		-5		/* Needs special comment in output */
#define MAXNAMELEN	20		/* Maximum Name Length */
#define MAXLDN		0xa		/* Biggest LDN */
#define LDNSIZE		(MAXLDN + 3)	/* Biggest LDN + 0 + NOLDN + EOT */
#define MAXNUMIDX	70		/* Maximum number of indexes */
#define IDXSIZE 	(MAXNUMIDX + 1)
#define MAXNUMPORTS	(2 + 1)		/* Maximum number of Super I/O ports */

/* Command line parameters. */
extern int dump, verbose;

struct superio_registers {
	int32_t superio_id; /* Signed, as we need EOT. */
	const char name[MAXNAMELEN];
	struct {
		int ldn;
		int idx[IDXSIZE];
		int def[IDXSIZE];
	} ldn[LDNSIZE];
};

/* superiotool.c */
uint8_t regval(uint16_t port, uint8_t reg);
void regwrite(uint16_t port, uint8_t reg, uint8_t val);
int superio_unknown(const struct superio_registers reg_table[], uint16_t id);
const char *get_superio_name(const struct superio_registers reg_table[],
			     uint16_t id);
void dump_superio(const char *name, const struct superio_registers reg_table[],
		  uint16_t port, uint16_t id);
void no_superio_found(uint16_t port);

/* fintek.c */
void dump_fintek(uint16_t port, uint16_t did);
void probe_idregs_fintek(uint16_t port);

/* ite.c */
void dump_ite(uint16_t port, uint16_t id);
void probe_idregs_ite(uint16_t port);

/* nsc.c */
void dump_ns8374(uint16_t port);
void probe_idregs_simple(uint16_t port);

/* smsc.c */
void probe_idregs_smsc(uint16_t port);

/* winbond.c */
void probe_idregs_winbond(uint16_t port);

/** Table of which config ports to probe on each Super I/O. */
const static struct {
	void (*probe_idregs) (uint16_t port);
	int ports[MAXNUMPORTS]; /* Signed, as we need EOT. */
} superio_ports_table[] = {
	{probe_idregs_simple,	{0x2e,	0x4e,	EOT}},
	{probe_idregs_fintek,	{0x2e,	0x4e,	EOT}},
	{probe_idregs_ite,	{0x2e,	0x4e,	EOT}},
	{probe_idregs_smsc,	{0x3f0,	0x370,	EOT}},
	{probe_idregs_winbond,	{0x2e,	0x4e,	EOT}},
};

#endif
