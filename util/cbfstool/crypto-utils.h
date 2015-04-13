/*
 * CBFS Image Manipulation
 *
 * Copyright (C) 2013 The Chromium OS Authors. All rights reserved.
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
 */

#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include "crypto.h"

#define MAGIC "CBFSKEYS"
#define VERSION1 0x00000001
#define VERSION VERSION1

struct cbkeys {
	uint8_t  	  magic[8];
	uint32_t 	  version;
	uint8_t 	  pk[crypto_sign_PUBLICKEYBYTES];
    uint8_t 	  sk[crypto_sign_SECRETKEYBYTES];
};

int write_cbkeys(const char *filename, const struct cbkeys *keypair);
int read_cbkeys(const char *filename, struct cbkeys *keypair);

#endif