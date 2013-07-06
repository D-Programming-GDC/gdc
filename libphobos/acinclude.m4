dnl Unix-specific configuration
AC_DEFUN([DPHOBOS_CONFIGURE_UNIX],[

AC_CHECK_HEADERS(pthread.h,:,
  [AC_MSG_ERROR([can't find pthread.h. Pthreads is the only supported thread library.])])

AC_MSG_CHECKING([for recursive mutex name])
AC_TRY_COMPILE([#include <pthread.h>],[
pthread_mutexattr_t attr;
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);],
  [AC_DEFINE(HAVE_PTHREAD_MUTEX_RECURSIVE,1,[Determines how to declared recursive mutexes])
   AC_MSG_RESULT([PTHREAD_MUTEX_RECURSIVE])],
  [AC_MSG_RESULT([PTHREAD_MUTEX_RECURSIVE_NP])])

AC_CHECK_TYPES([pthread_barrier_t, pthread_barrierattr_t,
		pthread_rwlock_t, pthread_rwlockattr_t,
		pthread_spinlock_t],,,[#include <pthread.h>])

AC_CHECK_TYPES([clockid_t],,,[#include <pthread.h>])

AC_SEARCH_LIBS(sem_init, pthread rt posix4)

AC_CHECK_HEADERS(semaphore.h)
AC_CHECK_FUNC(sem_init)
AC_CHECK_FUNC(semaphore_create)
AC_CHECK_FUNC(pthread_cond_wait)

if test -z "$d_sem_impl"; then
    # Probably need to test what actually works.  sem_init is defined
    # on AIX and Darwin but does not actually work.
    # For now, test for Mach semaphores first so it overrides Posix.  AIX
    # is a special case.
    if test "$ac_cv_func_semaphore_create" = "yes"; then
	d_sem_impl="mach"
    elif test "$ac_cv_func_sem_init" = "yes" && \
	    test "$ac_cv_header_semaphore_h" = "yes" && \
	    test -z "$d_is_aix"; then
	d_sem_impl="posix"
    elif test "$ac_cv_func_pthread_cond_wait" = "yes"; then
	d_sem_impl="pthreads"
    fi
fi

dnl TODO: change this to using pthreads? if so, define usepthreads
dnl and configure semaphore

case "$d_sem_impl" in
    posix) DCFG_SEMAPHORE_IMPL="GNU_Semaphore_POSIX" ;;
    mach)  DCFG_SEMAPHORE_IMPL="GNU_Semaphore_Mach"
	d_module_mach=1 ;;
    pthreads) DCFG_SEMAPHORE_IMPL="GNU_Sempahore_Pthreads" ;;
    skyos) DCFG_SEMAPHORE_IMPL="GNU_Sempahore_Pthreads"
	D_EXTRA_OBJS="$D_EXTRA_OBJS std/c/skyos/compat.o"
	;;
    *)  AC_MSG_ERROR([No usable semaphore implementation]) ;;
esac

AC_DEFINE(PHOBOS_USE_PTHREADS,1,[Define if using pthreads])


# Add "linux" module for compatibility even if not Linux
D_EXTRA_OBJS="std/c/linux/linux.o $D_EXTRA_OBJS"
DCFG_UNIX="Unix"
DCFG_POSIX="Posix"

])

