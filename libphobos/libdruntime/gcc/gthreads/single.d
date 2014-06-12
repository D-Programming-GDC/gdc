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
// This must match gthr-single.h

module gcc.gthreads.single;

// Just provide compatibility for mutex handling.

alias gthread_key_t   = int;
alias gthread_once_t  = int;
alias gthread_mutex_t = int;
alias gthread_recursive_mutex_t = int;

enum GTHREAD_MUTEX_INIT = 0;
enum GTHREAD_ONCE_INIT  = 0;
enum GTHREAD_RECURSIVE_MUTEX_INIT = 0;

// Backend thread functions
extern(C):
@trusted:
nothrow:

int gthread_active_p()
{
  return 0;
}

int gthread_once(gthread_once_t*, void function())
{
  return 0;
}

int gthread_key_create(gthread_key_t*, void* function(void*))
{
  return 0;
}

int gthread_key_delete(gthread_key_t)
{
  return 0;
}

void* gthread_getspecific(gthread_key_t)
{
  return null;
}

int gthread_setspecific(gthread_key_t, in void*)
{
  return 0;
}

void gthread_mutex_init(gthread_mutex_t* mutex)
{
  *(mutex) = GTHREAD_MUTEX_INIT;
}

int gthread_mutex_destroy(gthread_mutex_t*)
{
  return 0;
}

int gthread_mutex_lock(gthread_mutex_t*)
{
  return 0;
}

int gthread_mutex_trylock(gthread_mutex_t*)
{
  return 0;
}

int gthread_mutex_unlock(gthread_mutex_t*)
{
  return 0;
}

int gthread_recursive_mutex_init(gthread_mutex_t* mutex)
{
  gthread_mutex_init(mutex);
  return 0;
}

int gthread_recursive_mutex_lock(gthread_recursive_mutex_t* mutex)
{
  return gthread_mutex_lock(mutex);
}

int gthread_recursive_mutex_trylock(gthread_recursive_mutex_t* mutex)
{
  return gthread_mutex_trylock(mutex);
}

int gthread_recursive_mutex_unlock(gthread_recursive_mutex_t* mutex)
{
  return gthread_mutex_unlock(mutex);
}

int gthread_recursive_mutex_destroy(gthread_recursive_mutex_t* mutex)
{
  return gthread_mutex_destroy(mutex);
}

