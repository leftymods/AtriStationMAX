#ifndef __LINUX_COMPILER_H
#error "Please don't include <linux/compiler-gcc9.h> directly, include <linux/compiler.h> instead."
#endif

/* GCC 4.9+ specifics */
#if GCC_VERSION >= 40900
#define __copy(x) __attribute__((__copy__(x)))
#endif
