/* Written by Walter Bright, Christopher E. Miller, and many others.
 * http://www.digitalmars.com
 * Placed into public domain.
 */

module std.c.linux.pthread;

version (FreeBSD)
{
    public import std.c.freebsd.pthread;
}
else version (OpenBSD)
{
    public import std.c.openbsd.pthread;
}
else version (Solaris)
{
    public import std.c.solaris.pthread;
}
else
{

import std.c.linux.linux;

extern (C):

version(linux)
{
    /*  pthread declarations taken from pthread headers and
        http://svn.dsource.org/projects/bindings/trunk/pthreads.d
    */

    /* from bits/pthreadtypes.h
    */

    alias uint pthread_key_t;
    alias int pthread_once_t;

    struct _pthread_descr_struct
    {
    /*  Not defined in the headers ???
        Just needed here to typedef
        the _pthread_descr pointer
    */
    }

    typedef _pthread_descr_struct* _pthread_descr;

    struct _pthread_fastlock
    {
        int __status;
        int __spinlock;
    }

    typedef long __pthread_cond_align_t;

    /* from pthread.h
    */

    struct _pthread_cleanup_buffer
    {
	void function(void*) __routine;
	void* __arg;
	int __canceltype;
	_pthread_cleanup_buffer* __prev;
    }
}

version(OSX)
{
    /* from bits/types.h
    */

    // in std.c.linux.linux
    //typedef int __time_t;

    /* from time.h
    */

    /* from bits/pthreadtypes.h
    */
    private struct _opaque_pthread_t
    {
        int       __sig;
        void*     __cleanup_stack;
        byte[596] __opaque;
    }

    alias uint pthread_key_t;

    struct pthread_once_t
    {
        int     __sig;
        byte[4] __opaque;
    }

    /* from pthread.h
    */

    struct _pthread_cleanup_buffer
    {
        void function(void*)     __routine;
        void*                    __arg;
        _pthread_cleanup_buffer* __next;
    }
}

version(linux)
{
    int pthread_barrierattr_init(pthread_barrierattr_t*);
    int pthread_barrierattr_getpshared(pthread_barrierattr_t*, int*);
    int pthread_barrierattr_destroy(pthread_barrierattr_t*);
    int pthread_barrierattr_setpshared(pthread_barrierattr_t*, int);

    int pthread_barrier_init(pthread_barrier_t*, pthread_barrierattr_t*, uint);
    int pthread_barrier_destroy(pthread_barrier_t*);
    int pthread_barrier_wait(pthread_barrier_t*);
}

    int pthread_condattr_init(pthread_condattr_t*);
    int pthread_condattr_destroy(pthread_condattr_t*);
    int pthread_condattr_getpshared(pthread_condattr_t*, int*);
    int pthread_condattr_setpshared(pthread_condattr_t*, int);

    void pthread_exit(void*);
    int pthread_getattr_np(pthread_t, pthread_attr_t*);
    int pthread_getconcurrency();

version(linux)
{
    int pthread_getcpuclockid(pthread_t, clockid_t*);
}

    int pthread_mutexattr_getpshared(pthread_mutexattr_t*, int*);
    int pthread_mutexattr_setpshared(pthread_mutexattr_t*, int);
    int pthread_mutexattr_settype(pthread_mutexattr_t*, int);
    int pthread_mutexattr_gettype(pthread_mutexattr_t*, int*);
    int pthread_mutex_timedlock(pthread_mutex_t*, timespec*);
    int pthread_yield();

    int pthread_rwlock_init(pthread_rwlock_t*, pthread_rwlockattr_t*);
    int pthread_rwlock_destroy(pthread_rwlock_t*);
    int pthread_rwlock_rdlock(pthread_rwlock_t*);
    int pthread_rwlock_tryrdlock(pthread_rwlock_t*);
    int pthread_rwlock_timedrdlock(pthread_rwlock_t*, timespec*);
    int pthread_rwlock_wrlock(pthread_rwlock_t*);
    int pthread_rwlock_trywrlock(pthread_rwlock_t*);
    int pthread_rwlock_timedwrlock(pthread_rwlock_t*, timespec*);
    int pthread_rwlock_unlock(pthread_rwlock_t*);

    int pthread_rwlockattr_init(pthread_rwlockattr_t*);
    int pthread_rwlockattr_destroy(pthread_rwlockattr_t*);
    int pthread_rwlockattr_getpshared(pthread_rwlockattr_t*, int*);
    int pthread_rwlockattr_setpshared(pthread_rwlockattr_t*, int);
    int pthread_rwlockattr_getkind_np(pthread_rwlockattr_t*, int*);
    int pthread_rwlockattr_setkind_np(pthread_rwlockattr_t*, int);

version(linux)
{
    int pthread_spin_init(pthread_spinlock_t*, int);
    int pthread_spin_destroy(pthread_spinlock_t*);
    int pthread_spin_lock(pthread_spinlock_t*);
    int pthread_spin_trylock(pthread_spinlock_t*);
    int pthread_spin_unlock(pthread_spinlock_t*);
}

    void pthread_testcancel();
    int pthread_once(pthread_once_t*, void function());

    int pthread_atfork(void function(), void function(), void function());
    void pthread_kill_other_threads_np();
    int pthread_setschedparam(pthread_t, int, sched_param*);
    int pthread_getschedparam(pthread_t, int*, sched_param*);
    int pthread_cond_broadcast(pthread_cond_t*);
    int pthread_key_create(pthread_key_t*, void function(void*));
    int pthread_key_delete(pthread_key_t);
    int pthread_setconcurrency(int);
    int pthread_setspecific(pthread_key_t, void*);
    void* pthread_getspecific(pthread_key_t);

    void _pthread_cleanup_push(_pthread_cleanup_buffer*, void function(void*), void*);
    void _pthread_cleanup_push_defer(_pthread_cleanup_buffer*, void function(void*), void*);
    void _pthread_cleanup_pop(_pthread_cleanup_buffer*, int);
    void _pthread_cleanup_pop_restore(_pthread_cleanup_buffer*, int);

}
