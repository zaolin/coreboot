#ifndef PNP_INDEX_REG
#define PNP_INDEX_REG   0x15C
#endif
#ifndef PNP_DATA_REG
#define PNP_DATA_REG    0x15D
#endif
#ifndef SIO_COM1
#define SIO_COM1_BASE   0x3F8
#endif
#ifndef SIO_COM2
#define SIO_COM2_BASE   0x2F8
#endif

extern struct chip_control superio_winbond_w83627thf_control;

struct superio_winbond_w83627thf_config {
  struct com_ports com1, com2;
  
	struct lpt_ports lpt;
	int port;
};
