enum {
	PCI_BIOS_PRESENT	= 0xB101,
	FIND_PCI_DEVICE		= 0xB102,
	FIND_PCI_CLASS_CODE	= 0xB103,
	GENERATE_SPECIAL_CYCLE	= 0xB106,
	READ_CONFIG_BYTE	= 0xB108,
	READ_CONFIG_WORD	= 0xB109,
	READ_CONFIG_DWORD	= 0xB10A,
	WRITE_CONFIG_BYTE	= 0xB10B,
	WRITE_CONFIG_WORD	= 0xB10C,
	WRITE_CONFIG_DWORD	= 0xB10D,
	GET_IRQ_ROUTING_OPTIONS	= 0xB10E,
	SET_PCI_IRQ		= 0xB10F
};

enum {
	SUCCESSFUL		= 0x00,
	FUNC_NOT_SUPPORTED	= 0x81,
	BAD_VENDOR_ID		= 0x83,
	DEVICE_NOT_FOUND	= 0x86,
	BAD_REGISTER_NUMBER	= 0x87,
	SET_FAILED		= 0x88,
	BUFFER_TOO_SMALL	= 0x89
};
