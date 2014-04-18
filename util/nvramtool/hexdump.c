/*****************************************************************************\
 * hexdump.c
\*****************************************************************************/

#include "hexdump.h"

/* hexdump.c
 *
 * Copyright (C) 2002
 *     David S. Peterson.  All rights reserved.
 *
 * Author: David S. Peterson <dave_peterson@pobox.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the entire permission notice, including
 *    the following disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the entire permission notice,
 *    including the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 3. The name(s) of the author(s) may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * ALTERNATIVELY, this product may be distributed under the terms of the GNU
 * General Public License, in which case the provisions of the GPL are
 * required INSTEAD OF the above restrictions.  (This clause is necessary due
 * to a potential bad interaction between the GPL and the restrictions
 * contained in a BSD-style copyright.)
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

static void addrprint(FILE * outfile, uint64_t address, int width);
static void hexprint(FILE * outfile, unsigned char byte);
static void charprint(FILE * outfile, unsigned char byte,
		      unsigned char nonprintable,
		      is_printable_fn_t is_printable_fn);

/*--------------------------------------------------------------------------
 * hexdump
 *
 * Write a hex dump of 'mem' to 'outfile'.
 *
 * parameters:
 *     mem:             a pointer to the memory to display
 *     bytes:           the number of bytes of data to display
 *     addrprint_start: The address to associate with the first byte of
 *                      data.  For instance, a value of 0 indicates that the
 *                      first byte displayed should be labeled as byte 0.
 *     outfile:         The place where the hex dump should be written.
 *                      For instance, stdout or stderr may be passed here.
 *     format:          A structure specifying how the hex dump should be
 *                      formatted.
 *--------------------------------------------------------------------------*/
void hexdump(const void *mem, int bytes, uint64_t addrprint_start,
	     FILE * outfile, const hexdump_format_t * format)
{
	int bytes_left, index, i;
	const unsigned char *p;
	is_printable_fn_t is_printable_fn;

	/* Quietly return if the caller asks us to do something unreasonable. */
	if ((format->bytes_per_line <= 0) || (bytes < 0))
		return;

	is_printable_fn = format->is_printable_fn;

	if (is_printable_fn == NULL)
		is_printable_fn = default_is_printable_fn;

	p = (const unsigned char *)mem;
	index = 0;

	/* Each iteration handles one full line of output.  When loop
	 * terminates, the number of remaining bytes to display (if any)
	 * will not be enough to fill an entire line.
	 */
	for (bytes_left = bytes; 
			bytes_left >= format->bytes_per_line;
			bytes_left -= format->bytes_per_line) {
		/* print start address for current line */
		fprintf(outfile, format->indent);
		addrprint(outfile, addrprint_start + index,
			  format->addrprint_width);
		fprintf(outfile, format->sep1);

		/* display the bytes in hex */
		for (i = 0;;) {
			hexprint(outfile, p[index++]);

			if (++i >= format->bytes_per_line)
				break;

			fprintf(outfile, format->sep2);
		}

		index -= format->bytes_per_line;
		fprintf(outfile, format->sep3);

		/* display the bytes as characters */
		for (i = 0; i < format->bytes_per_line; i++)
			charprint(outfile, p[index++], format->nonprintable,
				  is_printable_fn);

		fprintf(outfile, "\n");
	}

	if (bytes_left == 0)
		return;

	/* print start address for last line */
	fprintf(outfile, format->indent);
	addrprint(outfile, addrprint_start + index, format->addrprint_width);
	fprintf(outfile, format->sep1);

	/* display bytes for last line in hex */
	for (i = 0; i < bytes_left; i++) {
		hexprint(outfile, p[index++]);
		fprintf(outfile, format->sep2);
	}

	index -= bytes_left;

	/* pad the rest of the hex byte area with spaces */
	for (;;) {
		fprintf(outfile, "  ");

		if (++i >= format->bytes_per_line)
			break;

		fprintf(outfile, format->sep2);
	}

	fprintf(outfile, format->sep3);

	/* display bytes for last line as characters */
	for (i = 0; i < bytes_left; i++)
		charprint(outfile, p[index++], format->nonprintable,
			  is_printable_fn);

	/* pad the rest of the character area with spaces */
	for (; i < format->bytes_per_line; i++)
		fprintf(outfile, " ");

	fprintf(outfile, "\n");
}

/*--------------------------------------------------------------------------
 * default_is_printable_fn
 *
 * Determine whether the input character is printable.  The proper behavior
 * for this type of function may be system-dependent.  This function takes a
 * conservative approach.  If it is not adequate for your purposes, you can
 * write your own.
 *
 * parameters:
 *     c: the input character
 *
 * return value:
 *     Return 1 if the input character is printable.  Otherwise return 0.
 *--------------------------------------------------------------------------*/
int default_is_printable_fn(unsigned char c)
{
	return (c >= 0x20) && (c <= 0x7e);
}

/*--------------------------------------------------------------------------
 * addrprint
 *
 * Display an address as a hexadecimal number.
 *
 * parameters:
 *     outfile: the place where the output should be written
 *     address: the address to display
 *     width:   The number of bytes wide the address should be displayed as.
 *              Must be a value from 1 to 8.
 *--------------------------------------------------------------------------*/
static void addrprint(FILE * outfile, uint64_t address, int width)
{
	char s[17];
	int i;

	/* force the user's input to be valid */
	if (width < 1)
		width = 1;
	else if (width > 8)
		width = 8;

	/* convert address to string */
	sprintf(s, "%016llx", (unsigned long long)address);

	/* write it out, with colons separating consecutive 16-bit
	 * chunks of the address
	 */
	for (i = 16 - (2 * width);;) {
		fprintf(outfile, "%c", s[i]);

		if (++i >= 16)
			break;

		if ((i % 4) == 0)
			fprintf(outfile, ":");
	}
}

/*--------------------------------------------------------------------------
 * hexprint
 *
 * Display a byte as a two digit hex value.
 *
 * parameters:
 *     outfile: the place where the output should be written
 *     byte:    the byte to display
 *--------------------------------------------------------------------------*/
static void hexprint(FILE * outfile, unsigned char byte)
{
	static const char tbl[] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
	};

	fprintf(outfile, "%c%c", tbl[byte >> 4], tbl[byte & 0x0f]);
}

/*--------------------------------------------------------------------------
 * charprint
 *
 * Display a byte as its character representation.
 *
 * parameters:
 *     outfile:         the place where the output should be written
 *     byte:            the byte to display
 *     nonprintable:    a substitute character to display if the byte
 *                      represents a nonprintable character
 *     is_printable_fn: a function that returns a boolean value indicating
 *                      whether a given character is printable
 *--------------------------------------------------------------------------*/
static void charprint(FILE * outfile, unsigned char byte,
		      unsigned char nonprintable,
		      is_printable_fn_t is_printable_fn)
{
	fprintf(outfile, "%c", is_printable_fn(byte) ? byte : nonprintable);
}
