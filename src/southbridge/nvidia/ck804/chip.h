#ifndef CK804_CHIP_H
#define CK804_CHIP_H

struct southbridge_nvidia_ck804_config 
{
        unsigned int ide0_enable : 1;
        unsigned int ide1_enable : 1;
	unsigned int sata0_enable : 1;
	unsigned int sata1_enable : 1;
	unsigned long nic_rom_address;
	unsigned long raid_rom_address;
	unsigned int mac_eeprom_smbus;
	unsigned int mac_eeprom_addr;
};
struct chip_operations;
extern struct chip_operations southbridge_nvidia_ck804_ops;

#endif /* CK804_CHIP_H */
