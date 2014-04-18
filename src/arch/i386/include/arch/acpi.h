/*
 * Initial LinuxBIOS ACPI Support - headers and defines.
 * 
 * written by Stefan Reinauer <stepan@openbios.org>
 * (C) 2004 SUSE LINUX AG
 *
 * The ACPI table structs are based on the Linux kernel sources.
 * 
 */
/* ACPI FADT & FACS added by Nick Barker <nick.barker9@btinternet.com>
 * those parts (C) 2004 Nick Barker
 */


#ifndef __ASM_ACPI_H
#define __ASM_ACPI_H

#if HAVE_ACPI_TABLES==1

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#define RSDP_NAME               "RSDP"
#define RSDP_SIG                "RSD PTR "  /* RSDT Pointer signature */

/* ACPI 2.0 table RSDP */

typedef struct acpi_rsdp {
	char  signature[8];
	u8    checksum;
	char  oem_id[6];
	u8    revision;
	u32   rsdt_address;
	u32   length;
	u64   xsdt_address;
	u8    ext_checksum;
	u8    reserved[3];
} __attribute__((packed)) acpi_rsdp_t;

/* Generic Address Container */

typedef struct acpi_gen_regaddr {
	u8  space_id;
	u8  bit_width;
	u8  bit_offset;
	u8  resv;
	u32 addrl;
	u32 addrh;
} __attribute__ ((packed)) acpi_addr_t;

/* Generic ACPI Header, provided by (almost) all tables */

typedef struct acpi_table_header         /* ACPI common table header */
{
	char signature [4];          /* ACPI signature (4 ASCII characters) */\
	u32  length;                 /* Length of table, in bytes, including header */\
	u8   revision;               /* ACPI Specification minor version # */\
	u8   checksum;               /* To make sum of entire table == 0 */\
	char oem_id [6];             /* OEM identification */\
	char oem_table_id [8];       /* OEM table identification */\
	u32  oem_revision;           /* OEM revision number */\
	char asl_compiler_id [4];    /* ASL compiler vendor ID */\
	u32  asl_compiler_revision;  /* ASL compiler revision number */
} __attribute__ ((packed)) acpi_header_t;

/* RSDT */

typedef struct acpi_rsdt {
	struct acpi_table_header header;
	u32 entry[8];
} __attribute__ ((packed)) acpi_rsdt_t;

/* HPET TIMERS */

typedef struct acpi_hpet {
	struct acpi_table_header header;
	u32 id;
	struct acpi_gen_regaddr addr;
	u8 number;
	u16 min_tick;
	u8 attributes;
} __attribute__ ((packed)) acpi_hpet_t;

typedef struct acpi_madt {
	struct acpi_table_header header;
	u32 lapic_addr;
	u32 flags;
} __attribute__ ((packed)) acpi_madt_t;

enum acpi_apic_types {
	LocalApic		= 0,
	IOApic			= 1,
	IRQSourceOverride	= 2,
	NMI			= 3,
	LocalApicNMI		= 4,
	LApicAddressOverride	= 5,
	IOSApic			= 6,
	LocalSApic		= 7,
	PlatformIRQSources	= 8
};

typedef struct acpi_madt_lapic {
	u8 type;
	u8 length;
	u8 processor_id;
	u8 apic_id;
	u32 flags;
} __attribute__ ((packed)) acpi_madt_lapic_t;

typedef struct acpi_madt_lapic_nmi {
	u8 type;
	u8 length;
	u8 processor_id;
	u16 flags;
	u8 lint;
} __attribute__ ((packed)) acpi_madt_lapic_nmi_t;


typedef struct acpi_madt_ioapic {
	u8 type;
	u8 length;
	u8 ioapic_id;
	u8 reserved;
	u32 ioapic_addr;
	u32 gsi_base;
} __attribute__ ((packed)) acpi_madt_ioapic_t;

typedef struct acpi_madt_irqoverride {
	u8 type;
	u8 length;
	u8 bus;
	u8 source;
	u32 gsirq;
	u16 flags;
} __attribute__ ((packed)) acpi_madt_irqoverride_t;


typedef struct acpi_fadt {
	struct acpi_table_header header;
	u32 firmware_ctrl;
	u32 dsdt;
	u8 res1;
	u8 preferred_pm_profile;
	u16 sci_int;
	u32 smi_cmd;
	u8 acpi_enable;
	u8 acpi_disable;
	u8 s4bios_req;
	u8 pstate_cnt;
	u32 pm1a_evt_blk;
	u32 pm1b_evt_blk;
	u32 pm1a_cnt_blk;
	u32 pm1b_cnt_blk;
	u32 pm2_cnt_blk;
	u32 pm_tmr_blk;
	u32 gpe0_blk;
	u32 gpe1_blk;
	u8 pm1_evt_len;
	u8 pm1_cnt_len;
	u8 pm2_cnt_len;
	u8 pm_tmr_len;
	u8 gpe0_blk_len;
	u8 gpe1_blk_len;
	u8 gpe1_base;
	u8 cst_cnt;
	u16 p_lvl2_lat;
	u16 p_lvl3_lat;
	u16 flush_size;
	u16 flush_stride;
	u8 duty_offset;
	u8 duty_width;
	u8 day_alrm;
	u8 mon_alrm;
	u8 century;
	u16 iapc_boot_arch;
	u8 res2;
	u32 flags;
	struct acpi_gen_regaddr reset_reg;
	u8 reset_value;
	u8 res3;
	u8 res4;
	u8 res5;
	u32 x_firmware_ctl_l;
	u32 x_firmware_ctl_h;
	u32 x_dsdt_l;
	u32 x_dsdt_h;
	struct acpi_gen_regaddr x_pm1a_evt_blk;
	struct acpi_gen_regaddr x_pm1b_evt_blk;
	struct acpi_gen_regaddr x_pm1a_cnt_blk;
	struct acpi_gen_regaddr	x_pm1b_cnt_blk;
	struct acpi_gen_regaddr x_pm2_cnt_blk;
	struct acpi_gen_regaddr x_pm_tmr_blk;
	struct acpi_gen_regaddr x_gpe0_blk;
	struct acpi_gen_regaddr x_gpe1_blk;
} __attribute__ ((packed)) acpi_fadt_t;

typedef struct acpi_facs {
	char signature[4];
	u32 length;
	u32 hardware_signature;
	u32 firmware_waking_vector;
	u32 global_lock;
	u32 flags;
	u32 x_firmware_waking_vector_l;
	u32 x_firmware_waking_vector_h;
	u8 version;
	u8 resv[33];
} __attribute__ ((packed)) acpi_facs_t;


unsigned long write_acpi_tables(unsigned long addr);

#else // HAVE_ACPI_TABLES

#define write_acpi_tables(start) (start)

#endif

#endif
