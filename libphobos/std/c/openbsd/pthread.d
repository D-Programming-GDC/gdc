/**
 * pthread for OpenBSD
 */

module std.c.openbsd.pthread;

version (OpenBSD) { } else { static assert(0); }

import std.c.openbsd.openbsd;

extern (C):



typedef void	*pthread_t;
typedef void	*pthread_attr_t;
typedef void	*pthread_mutex_t;
typedef void	*pthread_mutexattr_t;
typedef void	*pthread_cond_t;
typedef void	*pthread_condattr_t;
typedef int 	pthread_key_t;
typedef void	*pthread_rwlock_t;
typedef void	*pthread_rwlockattr_t;

alias void* pthread_addr_t;
alias void* function(void*) pthread_startroutine_t;

struct pthread_once_t
{
    int	state;
    pthread_mutex_t mutex;
}

enum pthread_mutextype
{
    PTHREAD_MUTEX_ERRORCHECK = 1,	
    PTHREAD_MUTEX_RECURSIVE = 2,	
    PTHREAD_MUTEX_NORMAL = 3,	
    PTHREAD_MUTEX_TYPE_MAX
}

int pthread_atfork(void function(), void function(), void function());
void pthread_cleanup_pop(int);
void pthread_cleanup_push(void function (void *), void *routine_arg);
int pthread_condattr_destroy(pthread_condattr_t *);
int pthread_condattr_init(pthread_condattr_t *);
void pthread_exit(void *);
void *pthread_getspecific(pthread_key_t);
int pthread_key_create(pthread_key_t *, void function (void *));
int pthread_key_delete(pthread_key_t);
int pthread_once(pthread_once_t *, void function ());
int pthread_rwlock_destroy(pthread_rwlock_t *);
int pthread_rwlock_init(pthread_rwlock_t *, pthread_rwlockattr_t *);
int pthread_rwlock_rdlock(pthread_rwlock_t *);
int pthread_rwlock_timedrdlock(pthread_rwlock_t *, timespec *);
int pthread_rwlock_timedwrlock(pthread_rwlock_t *, timespec *);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *);
int pthread_rwlock_trywrlock(pthread_rwlock_t *);
int pthread_rwlock_unlock(pthread_rwlock_t *);
int pthread_rwlock_wrlock(pthread_rwlock_t *);
int pthread_rwlockattr_init(pthread_rwlockattr_t *);
int pthread_rwlockattr_getpshared(pthread_rwlockattr_t *, int *);
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *, int);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *);
pthread_t thread_self();
int pthread_setspecific(pthread_key_t, __const void *);
int pthread_sigmask(int, sigset_t *, sigset_t *);
void pthread_testcancel();
int pthread_getprio(pthread_t);
int pthread_setprio(pthread_t, int);
void pthread_yield();
int pthread_mutexattr_getprioceiling(pthread_mutexattr_t *, int *);
int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *, int);
int pthread_mutex_getprioceiling(pthread_mutex_t *, int *);
int pthread_mutex_setprioceiling(pthread_mutex_t *, int, int *);
int pthread_mutexattr_getprotocol(pthread_mutexattr_t *, int *);
int pthread_mutexattr_setprotocol(pthread_mutexattr_t *, int);
int pthread_getschedparam(pthread_t pthread, int *, sched_param *);
int pthread_setschedparam(pthread_t, int, sched_param *);
int pthread_getconcurrency();
int pthread_setconcurrency(int);


