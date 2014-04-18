#ifndef LOGLEVEL_H
#define LOGLEVEL_H

/* Safe for inclusion in assembly */

#ifndef ASM_CONSOLE_LOGLEVEL
#define ASM_CONSOLE_LOGLEVEL CONFIG_MAXIMUM_CONSOLE_LOGLEVEL
#endif

#define BIOS_EMERG      0   /* system is unusable                   */
#define BIOS_ALERT      1   /* action must be taken immediately     */
#define BIOS_CRIT       2   /* critical conditions                  */
#define BIOS_ERR        3   /* error conditions                     */
#define BIOS_WARNING    4   /* warning conditions                   */
#define BIOS_NOTICE     5   /* normal but significant condition     */
#define BIOS_INFO       6   /* informational                        */
#define BIOS_DEBUG      7   /* debug-level messages                 */
#define BIOS_SPEW       8   /* Way too many details                 */

#endif /* LOGLEVEL_H */
