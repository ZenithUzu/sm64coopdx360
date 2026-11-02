#if defined(__XSDK__)
#include <errno.h>
#include <fcntl.h>
#include <sys/times.h>
#include <unistd.h>

/* xSDK implements unlinkat with the kernel directory-disposition option. */
int rmdir(const char *path) {
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
