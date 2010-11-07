#include <time.h>
#include "config.h"

time_t _d_gnu_cbridge_tza()
{
    time_t t;
    struct tm * p_tm;

    time(&t);
    p_tm = localtime(&t);       /* this will set timezone */

#if defined(HAVE_TIMEZONE)
    return - timezone;
#elif defined(HAVE__TIMEZONE)
    return - _timezone;
#elif HAVE_TM_GMTOFF_AND_ZONE
    /* std.date expects this value to not include
       the daylight saving time offset. */
    if (p_tm->tm_isdst)
        /* std.date assumes daylight saving time is a one hour offset,
           so no attempt is made determine the correct offset */
        return p_tm->tm_gmtoff - 3600;
    return p_tm->tm_gmtoff;
#else
    return (time_t) 0;
#endif
}
