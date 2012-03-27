#include "config.h"

#ifdef HAVE_GLIBC_STRERROR_R
#define _D_GNU_SOURCE 1
#endif

#include <string.h>
#include <sys/types.h>

char * _d_gnu_cbridge_strerror(int en, char * buf, size_t len)
{
#ifdef HAVE_GLIBC_STRERROR_R
    return strerror_r(en, buf, len);
#elif HAVE_STRERROR_R
    strerror_r(en, buf, len);
    return buf;
#else
    strncpy(buf, strerror(en), len);
    if (len)
        buf[len-1] = '\0';
    return buf;
#endif
}
