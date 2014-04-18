#include <console/console.h>
#include <ip_checksum.h>
#include <boot/linuxbios_tables.h>
#include "linuxbios_table.h"
#include <string.h>
#include <version.h>
#include <device/device.h>
#include <stdlib.h>

struct lb_header *lb_table_init(unsigned long addr)
{
	struct lb_header *header;

	/* 16 byte align the address */
	addr += 15;
	addr &= ~15;

	header = (void *)addr;
	header->signature[0] = 'L';
	header->signature[1] = 'B';
	header->signature[2] = 'I';
	header->signature[3] = 'O';
	header->header_bytes = sizeof(*header);
	header->header_checksum = 0;
	header->table_bytes = 0;
	header->table_checksum = 0;
	header->table_entries = 0;
	return header;
}

struct lb_record *lb_first_record(struct lb_header *header)
{
	struct lb_record *rec;
	rec = (void *)(((char *)header) + sizeof(*header));
	return rec;
}

struct lb_record *lb_last_record(struct lb_header *header)
{
	struct lb_record *rec;
	rec = (void *)(((char *)header) + sizeof(*header) + header->table_bytes);
	return rec;
}

struct lb_record *lb_next_record(struct lb_record *rec)
{
	rec = (void *)(((char *)rec) + rec->size);	
	return rec;
}

struct lb_record *lb_new_record(struct lb_header *header)
{
	struct lb_record *rec;
	rec = lb_last_record(header);
	if (header->table_entries) {
		header->table_bytes += rec->size;
	}
	rec = lb_last_record(header);
	header->table_entries++;
	rec->tag = LB_TAG_UNUSED;
	rec->size = sizeof(*rec);
	return rec;
}


struct lb_memory *lb_memory(struct lb_header *header)
{
	struct lb_record *rec;
	struct lb_memory *mem;
	rec = lb_new_record(header);
	mem = (struct lb_memory *)rec;
	mem->tag = LB_TAG_MEMORY;
	mem->size = sizeof(*mem);
	return mem;
}

struct lb_mainboard *lb_mainboard(struct lb_header *header)
{
	struct lb_record *rec;
	struct lb_mainboard *mainboard;
	rec = lb_new_record(header);
	mainboard = (struct lb_mainboard *)rec;
	mainboard->tag = LB_TAG_MAINBOARD;

	mainboard->size = (sizeof(*mainboard) +
		strlen(mainboard_vendor) + 1 + 
		strlen(mainboard_part_number) + 1 +
		3) & ~3;

	mainboard->vendor_idx = 0;
	mainboard->part_number_idx = strlen(mainboard_vendor) + 1;

	memcpy(mainboard->strings + mainboard->vendor_idx,
		mainboard_vendor,      strlen(mainboard_vendor) + 1);
	memcpy(mainboard->strings + mainboard->part_number_idx,
		mainboard_part_number, strlen(mainboard_part_number) + 1);

	return mainboard;
}

void lb_strings(struct lb_header *header)
{
	static const struct {
		uint32_t tag;
		const uint8_t *string;
	} strings[] = {
		{ LB_TAG_VERSION,        linuxbios_version,        },
		{ LB_TAG_EXTRA_VERSION,  linuxbios_extra_version,  },
		{ LB_TAG_BUILD,          linuxbios_build,          },
		{ LB_TAG_COMPILE_TIME,   linuxbios_compile_time,   },
		{ LB_TAG_COMPILE_BY,     linuxbios_compile_by,     },
		{ LB_TAG_COMPILE_HOST,   linuxbios_compile_host,   },
		{ LB_TAG_COMPILE_DOMAIN, linuxbios_compile_domain, },
		{ LB_TAG_COMPILER,       linuxbios_compiler,       },
		{ LB_TAG_LINKER,         linuxbios_linker,         },
		{ LB_TAG_ASSEMBLER,      linuxbios_assembler,      },
	};
	unsigned int i;
	for(i = 0; i < sizeof(strings)/sizeof(strings[0]); i++) {
		struct lb_string *rec;
		size_t len;
		rec = (struct lb_string *)lb_new_record(header);
		len = strlen(strings[i].string);
		rec->tag = strings[i].tag;
		rec->size = (sizeof(*rec) + len + 1 + 3) & ~3;
		memcpy(rec->string, strings[i].string, len+1);
	}

}

void lb_memory_range(struct lb_memory *mem,
	uint32_t type, uint64_t start, uint64_t size)
{
	int entries;
	entries = (mem->size - sizeof(*mem))/sizeof(mem->map[0]);
	mem->map[entries].start = start;
	mem->map[entries].size = size;
	mem->map[entries].type = type;
	mem->size += sizeof(mem->map[0]);
}

static void lb_reserve_table_memory(struct lb_header *head)
{
	struct lb_record *last_rec;
	struct lb_memory *mem;
	uint64_t start;
	uint64_t end;
	int i, entries;

	last_rec = lb_last_record(head);
	mem = get_lb_mem();
	start = (unsigned long)head;
	end = (unsigned long)last_rec;
	entries = (mem->size - sizeof(*mem))/sizeof(mem->map[0]);
	/* Resize the right two memory areas so this table is in
	 * a reserved area of memory.  Everything has been carefully
	 * setup so that is all we need to do.
	 */
	for(i = 0; i < entries; i++ ) {
		uint64_t map_start = mem->map[i].start;
		uint64_t map_end = map_start + mem->map[i].size;
		/* Does this area need to be expanded? */
		if (map_end == start) {
			mem->map[i].size = end - map_start;
		}
		/* Does this area need to be contracted? */
		else if (map_start == start) {
			mem->map[i].start = end;
			mem->map[i].size = map_end - end;
		}
	}
}

unsigned long lb_table_fini(struct lb_header *head)
{
	struct lb_record *rec, *first_rec;
	rec = lb_last_record(head);
	if (head->table_entries) {
		head->table_bytes += rec->size;
	}
	lb_reserve_table_memory(head);
	first_rec = lb_first_record(head);
	head->table_checksum = compute_ip_checksum(first_rec, head->table_bytes);
	head->header_checksum = 0;
	head->header_checksum = compute_ip_checksum(head, sizeof(*head));
	printk_debug("Wrote linuxbios table at: %p - %p  checksum %lx\n",
		head, rec, head->table_checksum);
	return (unsigned long)rec;
}

static void lb_cleanup_memory_ranges(struct lb_memory *mem)
{
	int entries;
	int i, j;
	entries = (mem->size - sizeof(*mem))/sizeof(mem->map[0]);
	
	/* Sort the lb memory ranges */
	for(i = 0; i < entries; i++) {
		for(j = i; j < entries; j++) {
			if (mem->map[j].start < mem->map[i].start) {
				struct lb_memory_range tmp;
				tmp = mem->map[i];
				mem->map[i] = mem->map[j];
				mem->map[j] = tmp;
			}
		}
	}

	/* Merge adjacent entries */
	for(i = 0; (i + 1) < entries; i++) {
		uint64_t start, end, nstart, nend;
		if (mem->map[i].type != mem->map[i + 1].type) {
			continue;
		}
		start  = mem->map[i].start;
		end    = start + mem->map[i].size;
		nstart = mem->map[i + 1].start;
		nend   = nstart + mem->map[i + 1].size;
		if ((start <= nstart) && (end > nstart)) {
			if (start > nstart) {
				start = nstart;
			}
			if (end < nend) {
				end = nend;
			}
			/* Record the new region size */
			mem->map[i].start = start;
			mem->map[i].size  = end - start;

			/* Delete the entry I have merged with */
			memmove(&mem->map[i + 1], &mem->map[i + 2], 
				((entries - i - 2) * sizeof(mem->map[0])));
			mem->size -= sizeof(mem->map[0]);
			entries -= 1;
			/* See if I can merge with the next entry as well */
			i -= 1; 
		}
	}
}

static void lb_remove_memory_range(struct lb_memory *mem, 
	uint64_t start, uint64_t size)
{
	uint64_t end;
	int entries;
	int i;

	end = start + size;
	entries = (mem->size - sizeof(*mem))/sizeof(mem->map[0]);

	/* Remove a reserved area from the memory map */
	for(i = 0; i < entries; i++) {
		uint64_t map_start = mem->map[i].start;
		uint64_t map_end   = map_start + mem->map[i].size;
		if ((start <= map_start) && (end >= map_end)) {
			/* Remove the completely covered range */
			memmove(&mem->map[i], &mem->map[i + 1], 
				((entries - i - 1) * sizeof(mem->map[0])));
			mem->size -= sizeof(mem->map[0]);
			entries -= 1;
			/* Since the index will disappear revisit what will appear here */
			i -= 1; 
		}
		else if ((start > map_start) && (end < map_end)) {
			/* Split the memory range */
			memmove(&mem->map[i + 1], &mem->map[i], 
				((entries - i) * sizeof(mem->map[0])));
			mem->size += sizeof(mem->map[0]);
			entries += 1;
			/* Update the first map entry */
			mem->map[i].size = start - map_start;
			/* Update the second map entry */
			mem->map[i + 1].start = end;
			mem->map[i + 1].size  = map_end - end;
			/* Don't bother with this map entry again */
			i += 1;
		}
		else if ((start <= map_start) && (end > map_start)) {
			/* Shrink the start of the memory range */
			mem->map[i].start = end;
			mem->map[i].size  = map_end - end;
		}
		else if ((start < map_end) && (start > map_start)) {
			/* Shrink the end of the memory range */
			mem->map[i].size = start - map_start;
		}
	}
}

static void lb_add_memory_range(struct lb_memory *mem,
	uint32_t type, uint64_t start, uint64_t size)
{
	lb_remove_memory_range(mem, start, size);
	lb_memory_range(mem, type, start, size);
	lb_cleanup_memory_ranges(mem);
}

/* Routines to extract part so the linuxBIOS table or 
 * information from the linuxBIOS table after we have written it.
 * Currently get_lb_mem relies on a global we can change the
 * implementaiton.
 */
static struct lb_memory *mem_ranges = 0;
struct lb_memory *get_lb_mem(void)
{
	return mem_ranges;
}

static struct lb_memory *build_lb_mem(struct lb_header *head)
{
	struct lb_memory *mem;
	struct device *dev;

	/* Record where the lb memory ranges will live */
	mem = lb_memory(head);
	mem_ranges = mem;

	/* Build the raw table of memory */
	for(dev = all_devices; dev; dev = dev->next) {
		struct resource *res, *last;
		last = &dev->resource[dev->resources];
		for(res = &dev->resource[0]; res < last; res++) {
			if (!(res->flags & IORESOURCE_MEM) ||
				!(res->flags & IORESOURCE_CACHEABLE)) {
				continue;
			}
			lb_memory_range(mem, LB_MEM_RAM, res->base, res->size);
		}
	}
	lb_cleanup_memory_ranges(mem);
	return mem;
}

unsigned long write_linuxbios_table( 
	unsigned long low_table_start, unsigned long low_table_end,
	unsigned long rom_table_startk, unsigned long rom_table_endk)
{
	unsigned long table_size;
	struct lb_header *head;
	struct lb_memory *mem;

	head = lb_table_init(low_table_end);
	low_table_end = (unsigned long)head;
	if (HAVE_OPTION_TABLE == 1) {
		struct lb_record *rec_dest, *rec_src;
		/* Write the option config table... */
		rec_dest = lb_new_record(head);
		rec_src = (struct lb_record *)&option_table;
		memcpy(rec_dest,  rec_src, rec_src->size);
	}
	/* Record where RAM is located */
	mem = build_lb_mem(head);
	
	/* Find the current mptable size */
	table_size = (low_table_end - low_table_start);

	/* Record the mptable and the the lb_table (This will be adjusted later) */
	lb_add_memory_range(mem, LB_MEM_TABLE, 
		low_table_start, table_size);

	/* Record the pirq table */
	lb_add_memory_range(mem, LB_MEM_TABLE, 
		rom_table_startk << 10, (rom_table_endk - rom_table_startk) << 10);

	/* Note:
	 * I assume that there is always memory at immediately after
	 * the low_table_end.  This means that after I setup the linuxbios table.
	 * I can trivially fixup the reserved memory ranges to hold the correct
	 * size of the linuxbios table.
	 */

	/* Record our motheboard */
	lb_mainboard(head);
	/* Record our various random string information */
	lb_strings(head);

	low_table_end = lb_table_fini(head);

	/* Remember where my valid memory ranges are */
	return low_table_end;
}
