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

#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "crypto.h"
#include "crypto-utils.h"

/* Random source for cryptographic data */
#define RAMDOM_DEVICE "/dev/random"

extern void randombytes(unsigned char *random, unsigned long long len) {
  int fd = open(RAMDOM_DEVICE, O_RDONLY);
  unsigned int i;
  int res = 0;

  if( fd < 0  ) {
    exit(-1);
  }

  for (i = 0; i < len; ++i) {
    res = read(fd, random + i, 1);

    if(res < 0) {
      close(fd);
      exit(-2);
    }
  }

  close(fd);

  return;
}

int write_cbkeys(const char *filename, const struct cbkeys *keypair) {
	FILE *fd = NULL;

	fd = fopen(filename, "w");

	fwrite( keypair, sizeof(struct cbkeys), 1, fd );

	fclose(fd);

	return 0;
}

int read_cbkeys(const char *filename, struct cbkeys *keypair) {
	struct cbkeys data;
	FILE *fd = NULL;

	fd = fopen(filename, "r");

	fread( &data, sizeof(struct cbkeys), 1, fd );
	fclose(fd);

	memcpy(keypair, &data, sizeof(struct cbkeys));

	return 0;
}