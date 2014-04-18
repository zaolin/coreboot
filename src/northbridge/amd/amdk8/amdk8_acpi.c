/*============================================================================
Copyright 2005 ADVANCED MICRO DEVICES, INC. All Rights Reserved.
This software and any related documentation (the "Materials") are the
confidential proprietary information of AMD. Unless otherwise provided in a
software agreement specifically licensing the Materials, the Materials are
provided in confidence and may not be distributed, modified, or reproduced in
whole or in part by any means.
LIMITATION OF LIABILITY: THE MATERIALS ARE PROVIDED "AS IS" WITHOUT ANY
EXPRESS OR IMPLIED WARRANTY OF ANY KIND, INCLUDING BUT NOT LIMITED TO
WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT, TITLE, FITNESS FOR ANY
PARTICULAR PURPOSE, OR WARRANTIES ARISING FROM CONDUCT, COURSE OF DEALING, OR
USAGE OF TRADE. IN NO EVENT SHALL AMD OR ITS LICENSORS BE LIABLE FOR ANY
DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF PROFITS,
BUSINESS INTERRUPTION, OR LOSS OF INFORMATION) ARISING OUT OF THE USE OF OR
INABILITY TO USE THE MATERIALS, EVEN IF AMD HAS BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES. BECAUSE SOME JURISDICTIONS PROHIBIT THE EXCLUSION
OR LIMITATION OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES, THE ABOVE
LIMITATION MAY NOT APPLY TO YOU.
AMD does not assume any responsibility for any errors which may appear in the
Materials nor any responsibility to support or update the Materials. AMD
retains the right to modify the Materials at any time, without notice, and is
not obligated to provide such modified Materials to you.
NO SUPPORT OBLIGATION: AMD is not obligated to furnish, support, or make any
further information, software, technical information, know-how, or show-how
available to you.
U.S. GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with "RESTRICTED
RIGHTS." Use, duplication, or disclosure by the Government is subject to the
restrictions as set forth in FAR 52.227-14 and DFAR 252.227-7013, et seq., or
its successor. Use of the Materials by the Government constitutes
acknowledgement of AMD's proprietary rights in them.
============================================================================*/
// 2005.9 serengeti support
// by yhlu
//

/*
 * 2005.9 yhlu add madt lapic creat dynamically and SRAT related
 */

#include <console/console.h>
#include <string.h>
#include <arch/acpi.h>
#include <device/pci.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/mtrr.h>
#include <cpu/amd/amdk8_sysconf.h>

//it seems these function can be moved arch/i386/boot/acpi.c

unsigned long acpi_create_madt_lapics(unsigned long current)
{
        device_t cpu;
        int cpu_index = 0;

        for(cpu = all_devices; cpu; cpu = cpu->next) {
                if ((cpu->path.type != DEVICE_PATH_APIC) ||
                        (cpu->bus->dev->path.type != DEVICE_PATH_APIC_CLUSTER))
                {
                        continue;
                }
                if (!cpu->enabled) {
                        continue;
                }
                current += acpi_create_madt_lapic((acpi_madt_lapic_t *)current, cpu_index, cpu->path.u.apic.apic_id);
                cpu_index++;

        }

        return current;
}

unsigned long acpi_create_madt_lapic_nmis(unsigned long current, u16 flags, u8 lint)
{
        device_t cpu;
        int cpu_index = 0;

        for(cpu = all_devices; cpu; cpu = cpu->next) {
                if ((cpu->path.type != DEVICE_PATH_APIC) ||
                        (cpu->bus->dev->path.type != DEVICE_PATH_APIC_CLUSTER))
                {
                        continue;
                }
                if (!cpu->enabled) {
                        continue;
                }
                current += acpi_create_madt_lapic_nmi((acpi_madt_lapic_nmi_t *)current, cpu_index, flags, lint);
                cpu_index++;

        }

        return current;
}
unsigned long acpi_create_srat_lapics(unsigned long current)
{
        device_t cpu;
        int cpu_index = 0;

        for(cpu = all_devices; cpu; cpu = cpu->next) {
                if ((cpu->path.type != DEVICE_PATH_APIC) ||
                        (cpu->bus->dev->path.type != DEVICE_PATH_APIC_CLUSTER))
                {
                        continue;
                }
                if (!cpu->enabled) {
                        continue;
                }
                printk_debug("SRAT: lapic cpu_index=%02x, node_id=%02x, apic_id=%02x\n", cpu_index, cpu->path.u.apic.node_id, cpu->path.u.apic.apic_id);
                current += acpi_create_srat_lapic((acpi_srat_lapic_t *)current, cpu->path.u.apic.node_id, cpu->path.u.apic.apic_id);
                cpu_index++;

        }

        return current;
}



static unsigned long resk(uint64_t value)
{
        unsigned long resultk;
        if (value < (1ULL << 42)) {
                resultk = value >> 10;
        }
        else {
                resultk = 0xffffffff;
        }
        return resultk;
}


struct acpi_srat_mem_state {
        unsigned long current;
};

void set_srat_mem(void *gp, struct device *dev, struct resource *res)
{
        struct acpi_srat_mem_state *state = gp;
        unsigned long basek, sizek;
        basek = resk(res->base);
        sizek = resk(res->size);

        printk_debug("set_srat_mem: dev %s, res->index=%04x startk=%08x, sizek=%08x\n",
                     dev_path(dev), res->index, basek, sizek);
        /*
                0-640K must be on node 0
                next range is from 1M---
                So will cut off before 1M in the mem range
        */
        if((basek+sizek)<1024) return;

        if(basek<1024) {
                sizek -= 1024 - basek;
                basek = 1024;
        }

        state->current += acpi_create_srat_mem((acpi_srat_mem_t *)state->current, (res->index & 0xf), basek, sizek, 1); // need to figure out NV
}

unsigned long acpi_fill_srat(unsigned long current)
{
        struct acpi_srat_mem_state srat_mem_state;

        /* create all subtables for processors */
        current = acpi_create_srat_lapics(current);

        /* create all subteble for memory range */

        /* 0-640K must be on node 0 */
        current += acpi_create_srat_mem((acpi_srat_mem_t *)current, 0, 0, 640, 1);//enable
#if 1
        srat_mem_state.current = current;
        search_global_resources(
                IORESOURCE_MEM | IORESOURCE_CACHEABLE, IORESOURCE_MEM | IORESOURCE_CACHEABLE,
                set_srat_mem, &srat_mem_state);

        current = srat_mem_state.current;
#endif
        return current;
}


unsigned long acpi_fill_slit(unsigned long current)
{
	/* need to find out the node num at first */
	/* fill the first 8 byte with that num */
	/* fill the next num*num byte with distance, local is 10, 1 hop mean 20, and 2 hop with 30.... */
	
	/* because We has assume that we know the topology of the HT connection, So we can have set if we know the node_num */
	static uint8_t hops_8[] = {   0, 1, 1, 2, 2, 3, 3, 4,
				      1, 0, 2, 1, 3, 2, 4, 3,
				      1, 2, 0, 1, 1, 2, 2, 3,
				      2, 1, 1, 0, 2, 1, 3, 2,
				      2, 3, 1, 2, 0, 1, 1, 2,
				      3, 2, 2, 1, 1, 0, 2, 1,
				      3, 4, 2, 3, 1, 2, 0, 1, 	 
				      4, 4, 3, 2, 2, 1, 1, 0 };

//	uint8_t outer_node[8];

	uint8_t *p = (uint8_t *)current;
	int nodes = sysconf.nodes;
	int i,j;
	memset(p, 0, 8+nodes*nodes);
//	memset((uint8_t *)outer_node, 0, 8);
	*p = (uint8_t) nodes;
	p += 8;

#if 0
	for(i=0;i<sysconf.hc_possible_num;i++) {
		if((sysconf.pci1234[i]&1) !=1 ) continue;
		outer_node[(sysconf.pci1234[i] >> 4) & 0xf] = 1; // mark the outer node		
	}
#endif
	
	for(i=0;i<nodes;i++) {
		for(j=0;j<nodes; j++) {
			if(i==j) { p[i*nodes+j] = 10; }
			else {
#if 0
				int k;
				uint8_t latency_factor = 0;
				int k_start, k_end;
				if(i<j) {
					k_start = i;
					k_end = j;
				} else {
					k_start = j;
					k_end = i;
				}
				for(k=k_start;k<=k_end; k++) {
					if(outer_node[k]) {
						latency_factor = 1;
						break;
					}
				}
				p[i*nodes+j] = hops_8[i*nodes+j] * 2 + latency_factor + 10;
#else	
				p[i*nodes+j] = hops_8[i*nodes+j] * 2 + 10;
#endif


			}
		}
	}

        current += 8+nodes*nodes;

        return current;
}


//end


// moved from mb acpi_tables.c
static void int_to_stream(uint32_t val, uint8_t *dest)
{
        int i;
        for(i=0;i<4;i++) {
                *(dest+i) = (val >> (8*i)) & 0xff;
        }
}


// used by acpi_tables.h

void update_ssdt(void *ssdt)
{
        uint8_t *BUSN;
        uint8_t *MMIO;
        uint8_t *PCIO;
        uint8_t *SBLK;
        uint8_t *TOM1;
        uint8_t *SBDN;
        uint8_t *HCLK;
        uint8_t *HCDN;
	uint8_t *CBST;

        int i;
        device_t dev;
        uint32_t dword;
        msr_t msr;

        BUSN = ssdt+0x3a; //+5 will be next BUSN
        MMIO = ssdt+0x57; //+5 will be next MMIO
        PCIO = ssdt+0xaf; //+5 will be next PCIO
        SBLK = ssdt+0xdc; // one byte
        TOM1 = ssdt+0xe3; //
        SBDN = ssdt+0xed;//
        HCLK = ssdt+0xfa; //+5 will be next HCLK
        HCDN = ssdt+0x12a; //+5 will be next HCDN
	CBST = ssdt+0x157; //

        dev = dev_find_slot(0, PCI_DEVFN(0x18, 1));
        for(i=0;i<4;i++) {
                dword = pci_read_config32(dev, 0xe0+i*4);
                int_to_stream(dword, BUSN+i*5);
        }
        for(i=0;i<0x10;i++) {
                dword = pci_read_config32(dev, 0x80+i*4);
                int_to_stream(dword, MMIO+i*5);
        }
        for(i=0;i<0x08;i++) {
                dword = pci_read_config32(dev, 0xc0+i*4);
                int_to_stream(dword, PCIO+i*5);
        }

        *SBLK = (uint8_t)(sysconf.sblk);

        msr = rdmsr(TOP_MEM);
        int_to_stream(msr.lo, TOM1);

        for(i=0;i<sysconf.hc_possible_num;i++) {
                int_to_stream(sysconf.pci1234[i], HCLK + i*5);
                int_to_stream(sysconf.hcdn[i],    HCDN + i*5);
        }
        for(i=sysconf.hc_possible_num; i<HC_POSSIBLE_NUM; i++) { // in case we set array size to other than 8
                int_to_stream(0x00000000, HCLK + i*5);
                int_to_stream(0x20202020, HCDN + i*5);
        }

        int_to_stream(sysconf.sbdn, SBDN);

	if((sysconf.pci1234[0] >> 12) & 0xff) { //sb chain on  other than bus 0
		*CBST = (uint8_t) (0x0f);
	}
	else {
		*CBST = (uint8_t) (0x00);
	}

}

//end
