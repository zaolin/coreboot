/*
 * This file is part of the LinuxBIOS project.
 *
 * Copyright (C) 2007 Advanced Micro Devices, Inc.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */


static  void print_raminit(const char *strval, u32 val)
{
	printk_debug("%s%08x\n", strval, val);
}


#define RAMINIT_DEBUG 1


static  void print_tx(const char *strval, u32 val)
{
#if RAMINIT_DEBUG == 1
	print_raminit(strval, val);
#endif
}


static  void print_t(const char *strval)
{
#if RAMINIT_DEBUG == 1
	print_debug(strval);
#endif
}
#include "amdfam10.h"
#include "../amdmct/wrappers/mcti.h"
#include "../amdmct/amddefs.h"
#include "../amdmct/mct/mct_d.h"
#include "../amdmct/mct/mct_d_gcc.h"

#include "../amdmct/wrappers/mcti_d.c"
#include "../amdmct/mct/mct_d.c"


#include "../amdmct/mct/mctmtr_d.c"
#include "../amdmct/mct/mctcsi_d.c"
#include "../amdmct/mct/mctecc_d.c"
#include "../amdmct/mct/mctpro_d.c"
#include "../amdmct/mct/mctdqs_d.c"
#include "../amdmct/mct/mctsrc.c"
#include "../amdmct/mct/mctsrc1p.c"
#include "../amdmct/mct/mcttmrl.c"
#include "../amdmct/mct/mcthdi.c"
#include "../amdmct/mct/mctndi_d.c"
#include "../amdmct/mct/mctchi_d.c"

#if SYSTEM_TYPE == SERVER
//L1
#include "../amdmct/mct/mctardk3.c"
#elif SYSTEM_TYPE == DESKTOP
//AM2
#include "../amdmct/mct/mctardk4.c"
//#elif SYSTEM_TYPE == MOBILE
//s1g1
//#include "../amdmct/mct/mctardk5.c"
#endif

#include "../amdmct/mct/mct_fd.c"

int mctRead_SPD(u32 smaddr, u32 reg)
{
	return spd_read_byte(smaddr, reg);
}


void mctSMBhub_Init(u32 node)
{
	struct sys_info *sysinfo = (struct sys_info *)(DCACHE_RAM_BASE + DCACHE_RAM_SIZE - DCACHE_RAM_GLOBAL_VAR_SIZE);
	struct mem_controller *ctrl = &( sysinfo->ctrl[node] );
	activate_spd_rom(ctrl);
}


void mctGet_DIMMAddr(struct DCTStatStruc *pDCTstat, u32 node)
{
	int j;
	struct sys_info *sysinfo = (struct sys_info *)(DCACHE_RAM_BASE + DCACHE_RAM_SIZE - DCACHE_RAM_GLOBAL_VAR_SIZE);
	struct mem_controller *ctrl = &( sysinfo->ctrl[node] );

	for(j=0;j<DIMM_SOCKETS;j++) {
		pDCTstat->DIMMAddr[j*2] = ctrl->spd_addr[j] & 0xff;
		pDCTstat->DIMMAddr[j*2+1] = ctrl->spd_addr[DIMM_SOCKETS + j] & 0xff;
	}

}


u32 mctGetLogicalCPUID(u32 Node)
{
	u32 dev;
	u32 val, valx;
	u32 family, model, stepping;
	u32 ret;
	dev = PA_NBMISC(Node);
	val = Get_NB32(dev, 0xfc);
	print_debug("Family_Model:"); print_debug_hex32(val); print_debug("\n");

	family = ((val >> 8) & 0x0f) + ((val>>20) & 0xff);
	model = ((val>>4) & 0x0f) | ((val>>(16-4)) & 0xf0);
	stepping = val & 0xff;
	print_debug("Family:"); print_debug_hex8(family); print_debug("\t");
	print_debug("Model:"); print_debug_hex8(model); print_debug("\t");
	print_debug("Stepping:"); print_debug_hex8(stepping); print_debug("\n");

	valx = (family<<12) | (model<<4) | (stepping);
	print_debug("converted:"); print_debug_hex32(valx); print_debug("\n");

	switch (valx) {
	case 0x10000:
		ret = AMD_DR_A0A;
		break;
	case 0x10001:
		ret = AMD_DR_A1B;
		break;
	case 0x10002:
		ret = AMD_DR_A2;
		break;
	default:
		ret = 0;
	}

	return ret;
}


void raminit_amdmct(struct sys_info *sysinfo)
{
	struct MCTStatStruc *pMCTstat = &(sysinfo->MCTstat);
	struct DCTStatStruc *pDCTstatA = sysinfo->DCTstatA;

	print_debug("raminit_amdmct begin:\n");

	mctAutoInitMCT_D(pMCTstat, pDCTstatA);

	print_debug("raminit_amdmct end:\n");
}
