#ifndef LINUXBIOS_TABLE_H
#define LINUXBIOS_TABLE_H

#include <boot/linuxbios_tables.h>

struct mem_range;

/* This file holds function prototypes for building the linuxbios table. */
unsigned long write_linuxbios_table(
	unsigned long low_table_start, unsigned long low_table_end,
	unsigned long rom_table_start, unsigned long rom_table_end);

struct lb_header *lb_table_init(unsigned long addr);
struct lb_record *lb_first_record(struct lb_header *header);
struct lb_record *lb_last_record(struct lb_header *header);
struct lb_record *lb_next_record(struct lb_record *rec);
struct lb_record *lb_new_record(struct lb_header *header);
struct lb_memory *lb_memory(struct lb_header *header);
void lb_memory_range(struct lb_memory *mem, 
	uint32_t type, unsigned long startk, unsigned long sizek);
struct lb_mainboard *lb_mainboard(struct lb_header *header);
unsigned long lb_table_fini(struct lb_header *header);

/* Routines to extract part so the linuxBIOS table or information
 * from the linuxBIOS table.
 */
struct lb_memory *get_lb_mem(void);

extern struct cmos_option_table option_table;

#endif /* LINUXBIOS_TABLE_H */
