/* GDC -- D front-end for GCC
   Copyright (C) 2013 Free Software Foundation, Inc.

   GCC is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   GCC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.
*/

// GNU/GCC threads interface routines for D.
// This must match gthr-posix.h

module gcc.gthr_posix;

// POSIX threads specific definitions.
// Easy, since the interface is just one-to-one mapping.

import core.sys.posix.pthread;

alias gthread_t       = pthread_t;
alias gthread_key_t   = pthread_key_t;
alias gthread_once_t  = pthread_once_t;
alias gthread_mutex_t = pthread_mutex_t;
alias gthread_cond_t  = pthread_cond_t;
alias gthread_time_t  = timespec;

//enum GTHREAD_MUTEX_INIT = PTHREAD_MUTEX_INITIALIZER
//enum GTHREAD_ONCE_INIT = PTHREAD_ONCE_INIT

// Backend thread functions
extern(C):

// TODO: FreeBSD and Solaris exposes a dummy POSIX threads
// interface that will need to be handled here.
int gthread_active_p()
{
  return 1;
}

int gthread_create(gthread_t* id, void* function(void*) func, void* args)
{
  return pthread_create(id, null, func, args);
}

int gthread_join(gthread_t id, void** ptr)
{
  return pthread_join(id, ptr);
}

int gthread_detach(gthread_t id)
{
  return pthread_detach(id);
}

int gthread_equal(gthread_t t1, gthread_t t2)
{
  return pthread_equal(t1, t2);
}

gthread_t gthread_self()
{
  return pthread_self();
}

int gthread_yield()
{
  return sched_yield();
}

int gthread_once(gthread_once_t* once, void function() func)
{
  if (gthread_active_p())
    return pthread_once(once, func);
  else
    return -1;
}

int gthread_key_create(gthread_key_t* key, void function(void*) dtor)
{
  return pthread_key_create(key, dtor);
}

int gthread_key_delete(gthread_key_t key)
{
  return pthread_key_delete(key);
}

void* gthread_getspecific(gthread_key_t key)
{
  return pthread_getspecific(key);
}

int gthread_setspecific(gthread_key_t key, in void* ptr)
{
  return pthread_setspecific(key, ptr);
}

void gthread_mutex_init_function(gthread_mutex_t* mutex)
{
  if (gthread_active_p())
    pthread_mutex_init(mutex, null);
}

int gthread_mutex_destroy(gthread_mutex_t* mutex)
{
  if (gthread_active_p())
    return pthread_mutex_destroy(mutex);
  else
    return 0;
}

int gthread_mutex_lock(gthread_mutex_t* mutex)
{
  if (gthread_active_p())
    return pthread_mutex_lock(mutex);
  else
    return 0;
}

int gthread_mutex_trylock(gthread_mutex_t* mutex)
{
  if (gthread_active_p())
    return pthread_mutex_trylock(mutex);
  else
    return 0;
}

int gthread_mutex_timedlock(gthread_mutex_t* mutex, in gthread_time_t* timeout)
{
  if (gthread_active_p())
    return pthread_mutex_timedlock(mutex, timeout);
  else
    return 0;
}

int gthread_mutex_unlock(gthread_mutex_t* mutex)
{
  if (gthread_active_p())
    return pthread_mutex_unlock(mutex);
  else
    return 0;
}

int gthread_cond_broadcast(gthread_cond_t* cond)
{
  return pthread_cond_broadcast(cond);
}

int gthread_cond_signal(gthread_cond_t* cond)
{
  return pthread_cond_signal(cond);
}

int gthread_cond_wait(gthread_cond_t* cond, gthread_mutex_t* mutex)
{
  return pthread_cond_wait(cond, mutex);
}

int gthread_cond_timedwait(gthread_cond_t* cond, gthread_mutex_t* mutex,
                           in gthread_time_t* timeout)
{
  return pthread_cond_timedwait(cond, mutex, timeout);
}

int gthread_cond_destroy(gthread_cond_t* cond)
{
  return pthread_cond_destroy(cond);
}

