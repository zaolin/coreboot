#ifndef FALLBACK_H
#define FALLBACK_H

#if !defined(ASSEMBLY) && !defined(__PRE_RAM__)

void set_boot_successful(void);
void boot_successful(void);

#endif /* ASSEMBLY */

#define RTC_BOOT_BYTE	48

#endif /* FALLBACK_H */
