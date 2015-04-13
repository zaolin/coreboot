/*
* This file is part of the coreboot project.
*
* Copyright (C) 2015 Philipp Deppenwiese <philipp@deppenwiese.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of
* the License.
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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <security/platform.h>
#include <security/nacl.h>

#include <spi-generic.h>
#include <spi_flash.h>

#define PUBKEY_IDENT "pubkey"
#define BLOCK_SIZE 64000


static int cbfs_verify(const unsigned char* data, size_t len) {
	size_t file_len = 0;
	void *file = NULL;
	const struct cbfs_header *header;
	struct cbfs_media *image = NULL;
	unsigned long long signaturesize = 0;
	unsigned long long messagesize = 0;
	unsigned char digest[crypto_hash_BYTES];
	unsigned char message[CBFS_MAX_SIG_LENGTH];
	unsigned char signature[CBFS_MAX_SIG_LENGTH];

	header = cbfs_get_header(image);

	if(header == NULL) {
		return -1;
	}

	file = cbfs_get_file_content(image, PUBKEY_IDENT, CBFS_TYPE_RAW, &file_len);

	if(file == NULL || file_len <= 0) {
		return -1;
	}

/* How to do this read data and map cbfs_header ???
	if(header->version != CBFS_HEADER_VERSION3) {
		return -1;
	}

	signaturesize = header->signaturesize;
	memcpy(signature, header->signature, MAX_SIG_LENGTH);*/

/*
	header->signaturesize = 0;
	memset(header->signature, 0, MAX_SIG_LENGTH);

	if(crypto_hash(digest,
					(unsigned char*)image.buffer.data,
					image.buffer.size) != 0) {
		return -1;
	}
	*/

	if(crypto_sign_open(message, &messagesize, signature, signaturesize, (unsigned char*)file) != 0) {
		return -2;
	}

	if(messagesize == crypto_hash_BYTES && crypto_verify_64(message, digest) != 0) {
		return -3;
	}

	return 0;
}

static int cbfs_write_verify(const unsigned char* data, size_t len) {
	unsigned int i;
	struct spi_flash *flash = NULL;

	flash = spi_flash_probe(0, 0);

	if( flash == NULL ) {
		return -1;
	}

	for(i = 0; i < len; ++i) {
		char buf[BLOCK_SIZE];

		flash->read(flash, (i*BLOCK_SIZE), BLOCK_SIZE, buf);

		if(memcmp(data+(i*BLOCK_SIZE), buf, BLOCK_SIZE) != 0) {
			continue;
		}

		flash->erase(flash, (i*BLOCK_SIZE), BLOCK_SIZE);
		flash->write(flash, (i*BLOCK_SIZE), BLOCK_SIZE, data+(i*BLOCK_SIZE));

		flash->read(flash, (i*BLOCK_SIZE), BLOCK_SIZE, buf);

		if(memcmp(data+(i*BLOCK_SIZE), buf, BLOCK_SIZE) != 0) {
			return -2;
		}
	}

	return 0;
}

void smi_update() {
	size_t firmware_len = 0;
	unsigned char *firmware = NULL;


	if(cbfs_verify(firmware, firmware_len) != 0) {
		printk(BIOS_DEBUG, "verify failed.");
	}

	if(cbfs_write_verify(firmware, firmware_len) != 0) {
		printk(BIOS_DEBUG, "Something went wrong.");
	}
}