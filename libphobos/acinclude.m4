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

DCFG_PTHREAD_SUSPEND=
AC_SUBST(DCFG_PTHREAD_SUSPEND)

if true; then
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
      *)     AC_MSG_ERROR([No usable semaphore implementation]) ;;
    esac
else
    dnl Need to be able to query thread state for this method to be useful
    AC_CHECK_FUNC(pthread_suspend_np)
    AC_CHECK_FUNC(pthread_continue_np)

    if test "$ac_cv_func_pthread_suspend_np" = "yes" && \
       test "$ac_cv_func_pthread_continue_np" = "yes" ; then
	# TODO: need to test that these actually work.
	DCFG_PTHREAD_SUSPEND=GNU_pthread_suspend
    else
	AC_MSG_ERROR([TODO])
    fi
fi

AC_DEFINE(PHOBOS_USE_PTHREADS,1,[Define if using pthreads])

AC_CHECK_FUNC(mmap,DCFG_MMAP="GNU_Unix_Have_MMap",[])

AC_CHECK_FUNC(getpwnam_r,DCFG_GETPWNAM_R="GNU_Unix_Have_getpwnam_r",[])


# Add "linux" module for compatibility even if not Linux
D_EXTRA_OBJS="std/c/linux/linux.o $D_EXTRA_OBJS"
DCFG_UNIX="Unix"
DCFG_POSIX="Posix"

])

dnl Garbage collection configuration
dnl TODO: deprecate this and merge all OS specific bits into druntime.
AC_DEFUN([DPHOBOS_CONFIGURE_GC],[

dnl D_GC_MODULES=gc/gcgcc.o

d_gc_alloc=
d_gc_stack=
d_gc_data=

case "$d_target_os" in
  aix*)     d_gc_data="$d_gc_data GC_Use_Data_Fixed"
	    ;;
  cygwin*)  d_gc_data="$d_gc_data GC_Use_Data_Fixed"
	    ;;
  darwin*)  D_GC_MODULES="$D_GC_MODULES rt/memory_osx.o"
	    d_gc_stack=GC_Use_Stack_Fixed
	    d_gc_data="$d_gc_data GC_Use_Data_Dyld"
	    ;;
  freebsd*|k*bsd*-gnu)
	    d_gc_stack=GC_Use_Stack_FreeBSD
	    d_gc_data="$d_gc_data GC_Use_Data_Fixed"
	    dnl maybe just GC_Use_Stack_ExternC
	    ;;
  linux*)
  	    #d_gc_stack=GC_Use_Stack_Proc_Stat
	    d_gc_data="$d_gc_data GC_Use_Data_Fixed"
	    #have_proc_maps=1
	    ;;
  skyos*)   d_gc_data="$d_gc_data GC_Use_Data_Fixed"
	    ;;
  *)        dnl D_GC_MODULES=gc/gcgcc.o
            ;;
esac

if test -z "$d_gc_alloc"; then
    AC_CHECK_FUNC(mmap,d_gc_alloc=GC_Use_Alloc_MMap,[])
fi
if test -z "$d_gc_alloc"; then
    AC_CHECK_FUNC(valloc,d_gc_alloc=GC_Use_Alloc_Valloc,[])
fi
if test -z "$d_gc_alloc"; then
    # Use malloc as a fallback
    d_gc_alloc=GC_Use_Alloc_Malloc
fi
#if test -z "$d_gc_alloc"; then
#    AC_MSG_ERROR([No usable memory allocation routine])
#fi

if test -z "$d_gc_stack"; then
    AC_MSG_CHECKING([for __libc_stack_end])
    AC_TRY_LINK([],[
	extern long __libc_stack_end;
	return __libc_stack_end == 0;],
      [AC_MSG_RESULT(yes)
       d_gc_stack=GC_Use_Stack_GLibC],
      [AC_MSG_RESULT(no)])
fi
if test -z "$d_gc_stack"; then
    AC_MSG_CHECKING([for __get_stack_base])
    AC_TRY_LINK([],[
	int stack_size;
	extern void*  __get_stack_base(int  *p_stack_size);
	return __get_stack_base(&stack_size) == 0;],
      [AC_MSG_RESULT(yes)
       d_gc_stack=GC_Use_Stack_Bionic],
      [AC_MSG_RESULT(no)])
fi
dnl if test -z "$d_gc_stack"; then
dnl    d_gc_stack=GC_Use_Stack_Guess
dnl    D_GC_MODULES="$D_GC_MODULES gc/gc_guess_stack.o"
dnl fi
if test -z "$d_gc_stack"; then
    AC_MSG_ERROR([No usable stack origin information])
fi

dnl if test -z "$d_gc_data"; then
dnl     AC_MSG_CHECKING([for __data_start and _end])
dnl     AC_TRY_LINK([],[
dnl 	    extern int __data_start;
dnl 	    extern int _end;
dnl 	    return & _end - & __data_start;],
dnl 	[AC_MSG_RESULT(yes)
dnl 	 d_gc_data="$d_gc_data GC_Use_Data_Data_Start_End"],
dnl 	[AC_MSG_RESULT(no)])
dnl fi
if test -n "$have_proc_maps" && test "$enable_proc_maps" = auto; then
    enable_proc_maps=yes
fi
if test "$enable_proc_maps" = yes; then
    d_gc_data="$d_gc_data GC_Use_Data_Proc_Maps"
fi
if test -z "$d_gc_data"; then
    AC_MSG_ERROR([No usable data segment information])
fi

f="-fversion=$d_gc_alloc -fversion=$d_gc_stack"
for m in $d_gc_data; do f="$f -fversion=$m"; done
D_GC_FLAGS=$f

])
