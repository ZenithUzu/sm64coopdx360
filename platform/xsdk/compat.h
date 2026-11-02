#ifndef COOPDX_XSDK_COMPAT_H
#define COOPDX_XSDK_COMPAT_H
#include <limits.h>
/* This newlib's C++ limits.h names long long limits LONG_LONG_MAX/MIN.
 * Lua's public configuration expects the C99 spellings, with 64-bit integers. */
#ifndef LLONG_MAX
#define LLONG_MAX __LONG_LONG_MAX__
#endif
#ifndef LLONG_MIN
#define LLONG_MIN (-__LONG_LONG_MAX__ - 1LL)
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX (__LONG_LONG_MAX__ * 2ULL + 1ULL)
#endif
#endif
