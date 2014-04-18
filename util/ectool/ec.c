/*
 * This file is part of the ectool project.
 *
 * Copyright (C) 2008-2009 coresystems GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/io.h>
#include "ec.h"

extern int verbose;

#define debug(x...) if (verbose) printf(x)

int send_ec_command(uint8_t command)
{
	int timeout;

	timeout = 0x7ff;
	while ((inb(EC_SC) & EC_IBF) && --timeout) {
		usleep(10);
		if ((timeout & 0xff) == 0)
			debug(".");
	}
	if (!timeout) {
		printf("Timeout while sending command 0x%02x to EC!\n",
		       command);
		// return -1;
	}

	outb(command, EC_SC);
	return 0;
}

int send_ec_data(uint8_t data)
{
	int timeout;

	timeout = 0x7ff;
	while ((inb(EC_SC) & EC_IBF) && --timeout) {	// wait for IBF = 0
		usleep(10);
		if ((timeout & 0xff) == 0)
			debug(".");
	}
	if (!timeout) {
		printf("Timeout while sending data 0x%02x to EC!\n", data);
		// return -1;
	}

	outb(data, EC_DATA);

	return 0;
}

int send_ec_data_nowait(uint8_t data)
{
	outb(data, EC_DATA);

	return 0;
}

uint8_t recv_ec_data(void)
{
	int timeout;
	uint8_t data;

	timeout = 0x7fff;
	while (--timeout) {	// Wait for OBF = 1
		if (inb(EC_SC) & EC_OBF) {
			break;
		}
		usleep(10);
		if ((timeout & 0xff) == 0)
			debug(".");
	}
	if (!timeout) {
		printf("\nTimeout while receiving data from EC!\n");
		// return -1;
	}

	data = inb(EC_DATA);
	debug("recv_ec_data: 0x%02x\n", data);

	return data;
}

uint8_t ec_read(uint8_t addr)
{
	send_ec_command(0x80);
	send_ec_data(addr);

	return recv_ec_data();
}

int ec_write(uint8_t addr, uint8_t data)
{
	send_ec_command(0x81);
	send_ec_data(addr);

	return send_ec_data(data);
}
