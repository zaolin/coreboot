#ifndef __W49F002U_H__
#define __W49F002U_H__ 1

extern int probe_49f002 (struct flashchip * flash);
extern int erase_49f002 (struct flashchip * flash);
extern int write_49f002 (struct flashchip * flash, unsigned char * buf);

#endif /* !__W49F002U_H__ */
