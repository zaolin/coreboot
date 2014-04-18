/*
 * rom-mkstage
 *
 * Copyright (C) 2008 Jordan Crouse <jordan@cosmicpenguin.net>
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
#include <unistd.h>
#include "elf.h"
#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>

#include "common.h"
#include "../cbfs.h"

int parse_elf(unsigned char *input, unsigned char **output,
	      int mode, void (*compress) (char *, int, char *, int *))
{
	Elf32_Phdr *phdr;
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *) input;
	Elf32_Shdr *shdr;
	char *header, *buffer;
	unsigned char *out;

	int headers;
	int i;
	struct cbfs_stage *stage;
	unsigned int data_start, data_end, mem_end;

	headers = ehdr->e_phnum;
	header = (char *)ehdr;

	phdr = (Elf32_Phdr *) & (header[ehdr->e_phoff]);
	shdr = (Elf32_Shdr *) & (header[ehdr->e_shoff]);

	/* Now, regular headers - we only care about PT_LOAD headers,
	 * because thats what we're actually going to load
	 */

	data_start = 0xFFFFFFFF;
	data_end = 0;
	mem_end = 0;

	for (i = 0; i < headers; i++) {
		unsigned int start, mend, rend;

		if (phdr[i].p_type != PT_LOAD)
			continue;

		/* Empty segments are never interesting */
		if (phdr[i].p_memsz == 0)
			continue;

		/* BSS */

		start = phdr[i].p_paddr;

		mend = start + phdr[i].p_memsz;
		rend = start + phdr[i].p_filesz;

		if (start < data_start)
			data_start = start;

		if (rend > data_end)
			data_end = rend;

		if (mend > mem_end)
			mem_end = mend;
	}

	/* allocate an intermediate buffer for the data */
	buffer = calloc(data_end - data_start, 1);

	if (buffer == NULL) {
		fprintf(stderr, "E: Unable to allocate memory: %m\n");
		return -1;
	}

	/* Copy the file data into the buffer */

	for (i = 0; i < headers; i++) {

		if (phdr[i].p_type != PT_LOAD)
			continue;

		if (phdr[i].p_memsz == 0)
			continue;

		memcpy(buffer + (phdr[i].p_paddr - data_start),
		       &header[phdr[i].p_offset], phdr[i].p_filesz);
	}

	/* Now make the output buffer */
	out = calloc(sizeof(struct cbfs_stage) + data_end - data_start, 1);

	if (out == NULL) {
		fprintf(stderr, "E: Unable to allocate memory: %m\n");
		return -1;
	}

	stage = (struct cbfs_stage *)out;

	stage->load = data_start;
	stage->memlen = mem_end - data_start;
	stage->compression = mode;
	stage->entry = ehdr->e_entry;

	compress(buffer, data_end - data_start,
		 (char *)(out + sizeof(struct cbfs_stage)),
		 (int *)&stage->len);

	*output = out;

	return sizeof(struct cbfs_stage) + stage->len;
}

int main(int argc, char **argv)
{
	void (*compress) (char *, int, char *, int *);
	int algo = CBFS_COMPRESS_LZMA;

	char *output = NULL;
	char *input = NULL;

	unsigned char *buffer, *obuffer;
	int size, osize;

	while (1) {
		int option_index;
		static struct option longopt[] = {
			{"output", 1, 0, 'o'},
			{"lzma", 0, 0, 'l'},
			{"nocompress", 0, 0, 'n'},
		};

		signed char ch = getopt_long(argc, argv, "o:ln",
					     longopt, &option_index);

		if (ch == -1)
			break;

		switch (ch) {
		case 'o':
			output = optarg;
			break;
		case 'l':
			algo = CBFS_COMPRESS_LZMA;
			break;
		case 'n':
			algo = CBFS_COMPRESS_NONE;
			break;
		default:
			//usage();
			return -1;
		}
	}

	if (optind < argc)
		input = argv[optind];

	if (input == NULL || !strcmp(input, "-"))
		buffer = file_read_to_buffer(STDIN_FILENO, &size);
	else
		buffer = file_read(input, &size);

	if (!iself(buffer)) {
		fprintf(stderr, "E:  The incoming file is not an ELF\n");
		return -1;
	}

	switch (algo) {
	case CBFS_COMPRESS_NONE:
		compress = none_compress;
		break;
	case CBFS_COMPRESS_LZMA:
		compress = lzma_compress;
		break;
	}

	osize = parse_elf(buffer, &obuffer, algo, compress);

	if (osize == -1) {
		fprintf(stderr, "E:  Error while converting the ELF\n");
		return -1;
	}

	if (output == NULL || !strcmp(output, "-"))
		file_write_from_buffer(STDOUT_FILENO, obuffer, osize);
	else
		file_write(output, obuffer, osize);

	return 0;
}
