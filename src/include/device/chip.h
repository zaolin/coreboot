/* chips are arbitrary chips (superio, southbridge, etc.)
 * They have private structures that define chip resources and default 
 * settings. They have four externally visible functions for control. 
 * They have a generic component which applies to all chips for 
 * path, etc. 
 */

/* some of the types of resources chips can control */

struct com_ports {
  unsigned int enable,baud, base, irq;
};

/* lpt port description. 
 * Note that for many chips you only really need to define the 
 * enable. 
 */
struct lpt_ports {
	unsigned int enable, // 1 if this port is enabled
		     mode,   // pp mode
		     base,   // IO base of the parallel port 
                     irq;    // irq
};

enum chip_pass {
	CHIP_PRE_CONSOLE,
	CHIP_PRE_DEVICE_ENUMERATE,
	CHIP_PRE_DEVICE_CONFIGURE,
	CHIP_PRE_DEVICE_ENABLE,
	CHIP_PRE_DEVICE_INITIALIZE,
	CHIP_PRE_BOOT
};


/* linkages from devices of a type (e.g. superio devices) 
 * to the actual physical PCI device. This type is used in an array of 
 * structs built by NLBConfig.py. We owe this idea to Plan 9.
 */

struct chip;

/* there is one of these for each TYPE of chip */
struct chip_control {
  void (*enable)(struct chip *, enum chip_pass);
  char *path;     /* the default path. Can be overridden
		   * by commands in config
		   */
  // This is the print name for debugging
  char *name;
};

struct chip {
  struct chip_control *control; /* for this device */
  char *path; /* can be 0, in which case the default is taken */
  char *configuration; /* can be 0. */
  int irq;
  struct chip *next, *children;
  /* there is one of these for each INSTANCE of a chip */
  void *chip_info; /* the dreaded "void *" */
};

extern struct chip *root;
extern void chip_configure(struct chip *, enum chip_pass);
