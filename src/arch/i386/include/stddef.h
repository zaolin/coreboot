#ifndef I386_STDDEF_H
#define I386_STDDEF_H

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;

typedef int wchar_t;
typedef unsigned int wint_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#endif /* I386_STDDEF_H */
