/*****************************************************************************\
 * cmos_ops.h
 * $Id$
 *****************************************************************************
 *  Copyright (C) 2002-2005 The Regents of the University of California.
 *  Produced at the Lawrence Livermore National Laboratory.
 *  Written by David S. Peterson <dsp@llnl.gov> <dave_peterson@pobox.com>.
 *  UCRL-CODE-2003-012
 *  All rights reserved.
 *
 *  This file is part of lxbios, a utility for reading/writing coreboot
 *  parameters and displaying information from the coreboot table.
 *  For details, see http://coreboot.org/Lxbios.
 *
 *  Please also read the file DISCLAIMER which is included in this software
 *  distribution.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License (as published by the
 *  Free Software Foundation) version 2, dated June 1991.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the terms and
 *  conditions of the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
\*****************************************************************************/

#ifndef LXBIOS_CMOS_OPS_H
#define LXBIOS_CMOS_OPS_H

#include "common.h"
#include "layout.h"

#define CMOS_OP_BAD_ENUM_VALUE (CMOS_OP_RESULT_START + 0)
#define CMOS_OP_NEGATIVE_INT (CMOS_OP_RESULT_START + 1)
#define CMOS_OP_INVALID_INT (CMOS_OP_RESULT_START + 2)
#define CMOS_OP_RESERVED (CMOS_OP_RESULT_START + 3)
#define CMOS_OP_VALUE_TOO_WIDE (CMOS_OP_RESULT_START + 4)
#define CMOS_OP_NO_MATCHING_ENUM (CMOS_OP_RESULT_START + 5)

int prepare_cmos_read (const cmos_entry_t *e);
int prepare_cmos_write (const cmos_entry_t *e, const char value_str[],
                        unsigned long long *value);
uint16_t cmos_checksum_read (void);
void cmos_checksum_write (uint16_t checksum);
uint16_t cmos_checksum_compute (void);
void cmos_checksum_verify (void);

#endif  /* LXBIOS_CMOS_OPS_H */
