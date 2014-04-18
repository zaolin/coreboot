#ifndef PART_FALLBACK_BOOT_H
#define PART_FALLBACK_BOOT_H

#ifndef ASSEMBLY

#if HAVE_FALLBACK_BOOT
void boot_successful(void);
#else
#define boot_successful()
#endif

#endif /* ASSEMBLY */

#define RTC_BOOT_BYTE	48

#endif /* PART_FALLBACK_BOOT_H */
