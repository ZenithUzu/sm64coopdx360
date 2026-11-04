#if defined(__XSDK__)
#include <errno.h>
#include <fcntl.h>
#include <sys/times.h>
#include <unistd.h>

/* Older xSDK revisions need this fallback. Newer SDKs provide rmdir in path.c;
 * their strong definition must take precedence when that object is linked. */
__attribute__((weak)) int rmdir(const char *path) {
    return unlinkat(AT_FDCWD, path, AT_REMOVEDIR);
}

/* There is no process CPU-accounting API in this xSDK revision. Report the
 * standard failure rather than supplying wall time as CPU time to Lua clock(). */
clock_t times(struct tms *buffer) {
    (void)buffer;
    errno = ENOSYS;
    return (clock_t)-1;
}
#endif
