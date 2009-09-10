#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>

int _d_gcc_gc_freebsd_stack(void ** out_origin)
{
    int nm[2] = {CTL_KERN, KERN_USRSTACK};
    size_t len = sizeof(void *);
    int r = sysctl(nm, 2, out_origin, &len, NULL, 0);
    
    return ! r;
}
