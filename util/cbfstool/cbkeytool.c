/*
 * cbfstool, CLI utility for CBFS keypair generation and test verification
 *
 * Copyright (C) 2015 Philipp Deppenwiese <philipp@deppenwiese.net>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <stddef.h>

#include "common.h"
#include "crypto.h"
#include "crypto-utils.h"
#include "cbfs.h"
#include "cbfs_image.h"
#include "fit.h"

struct command {
	const char *name;
	const char *optstring;
	int (*function) (void);
};

static struct param {
	char *keyfile;
	char *cbfs;
	int verbose;
} param = {
	.verbose = 0,
};

static int gen_keypair(void)
{
	struct cbkeys file;
	struct stat st;
	char filename[NAME_MAX];

	strncpy((char*)file.magic, MAGIC, strlen(MAGIC));

	file.version = VERSION;

    if(stat(param.keyfile, &st) == 0) {
    	return -1;
    }

    sprintf(filename, "%s%s", param.keyfile, ".eckp");

	if(crypto_sign_keypair(file.pk, file.sk) < 0) {
		return -2;
	}

	if(write_cbkeys(filename, &file) < 0) {
		return -3;
	}

	return 0;
}

static int cbfs_verify(void)
{
	struct cbkeys file;
	struct cbfs_image image;
	struct stat st;
	struct buffer outheader;
	void *sig_loc = NULL;
	unsigned long long signaturesize = 0;
	unsigned long long messagesize = 0;
	unsigned char digest[crypto_hash_BYTES];
	unsigned char message[CBFS_MAX_SIG_LENGTH];
	unsigned char signature[CBFS_MAX_SIG_LENGTH];
	unsigned char default_signature[CBFS_MAX_SIG_LENGTH];

    if(stat(param.keyfile, &st) != 0) {
    	return -1;
    }

    if(stat(param.cbfs, &st) != 0) {
    	return -1;
    }

    if(read_cbkeys(param.keyfile, &file) < 0) {
		return -2;
	}

	if (cbfs_image_from_file(&image, param.cbfs) != 0) {
		ERROR("Could not load ROM image '%s'.\n", param.cbfs);
		return -2;
	}

	sig_loc = cbfs_find_header(image.buffer.data, image.buffer.size);
	sig_loc += offsetof(struct cbfs_header, signaturesize);

	outheader.data = sig_loc;
	outheader.size = 0;

	signaturesize = xdr_be.get32(&outheader);
	bgets(&outheader, signature, CBFS_MAX_SIG_LENGTH);

	outheader.data = sig_loc;
	outheader.size = 0;

	memset(default_signature, 0, CBFS_MAX_SIG_LENGTH);

	xdr_be.put32(&outheader, 0);
	bputs(&outheader, default_signature, CBFS_MAX_SIG_LENGTH);

	if(crypto_hash(digest,
					(unsigned char*)image.buffer.data,
					image.buffer.size) != 0) {
		return -3;
	}

	int ret = crypto_sign_open(message, &messagesize, signature, signaturesize, file.pk);
	if( ret != 0) {
		printf("signature verification failed: %i !\n", ret);
		return -4;
	}

	if(messagesize == crypto_hash_BYTES && crypto_verify_64(message, digest) != 0) {
		printf("digest doesn't match signature.");
		return -5;
	}

	printf("data could be verified !\n");

	return 0;
}

static const struct command commands[] = {
	{"create", "k:vh?", gen_keypair},
	{"verify", "f:k:vh?", cbfs_verify},
};

static struct option long_options[] = {
	{"cbfsfile",     required_argument, 0, 'f' },
	{"keyfile",      required_argument, 0, 'k' },
	{"verbose",      no_argument,       0, 'v' },
	{"help",         no_argument,       0, 'h' },
	{NULL,           0,                 0,  0  }
};

static void usage(char *name)
{
	printf
	    ("cbkeytool: Management utility for CBFS signature and keypair\n\n"
	     "USAGE:\n" " %s [-h]\n"
	     " %s COMMAND [-v] [PARAMETERS]...\n\n" "OPTIONs:\n"
	     "  -v              	Provide verbose output\n"
	     "  -h              	Display this help message\n\n"
	     "COMMANDs:\n"
	     " create -k FILE   		Create a new Ed25519 curve keypair\n"
	     " verify -k FILE -f FILE 	Verify coreboot.rom with given keyfile\n"
	     , name, name
	    );
}

int main(int argc, char **argv)
{
	size_t i;
	int c;

	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}

	char *cmd = argv[1];
	optind += 1;

	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(cmd, commands[i].name) != 0)
			continue;

		while (1) {
			int option_index = 0;

			c = getopt_long(argc, argv, commands[i].optstring,
						long_options, &option_index);
			if (c == -1)
				break;

			/* filter out illegal long options */
			if (strchr(commands[i].optstring, c) == NULL) {
				/* TODO maybe print actual long option instead */
				ERROR("%s: invalid option -- '%c'\n",
				      argv[0], c);
				c = '?';
			}

			switch(c) {
			case 'k':
				param.keyfile = optarg;
				break;
			case 'f':
				param.cbfs = optarg;
				break;
			case 'v':
				param.verbose++;
				break;
			case 'h':
			case '?':
				usage(argv[0]);
				return 1;
			default:
				break;
			}
		}

		return commands[i].function();
	}

	ERROR("Unknown command '%s'.\n", cmd);
	usage(argv[0]);
	return 1;
}