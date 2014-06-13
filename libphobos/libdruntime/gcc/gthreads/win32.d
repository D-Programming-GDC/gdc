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
// This must match gthr-win32.h

module gcc.gthreads.win32;

/* Windows32 threads specific definitions. The windows32 threading model
   does not map well into pthread-inspired gcc's threading model, and so
   there are caveats one needs to be aware of.

   1. The destructor supplied to gthread_key_create is ignored for
      generic x86-win32 ports.

      However, Mingw runtime (version 0.3 or newer) provides a mechanism
      to emulate pthreads key dtors; the runtime provides a special DLL,
      linked in if -mthreads option is specified, that runs the dtors in
      the reverse order of registration when each thread exits. If
      -mthreads option is not given, a stub is linked in instead of the
      DLL, which results in memory leak. Other x86-win32 ports can use
      the same technique of course to avoid the leak.

   2. The error codes returned are non-POSIX like, and cast into ints.
      This may cause incorrect error return due to truncation values on
      hw where DWORD.sizeof > int.sizeof.

   The basic framework should work well enough. In the long term, GCC
   needs to use Structured Exception Handling on Windows32.  */

import core.sys.windows.windows;
import core.stdc.errno;

alias gthread_key_t   = ULONG;

struct gthread_once_t
{
  INT done;
  LONG started;
}

alias gthread_mutex_t = CRITICAL_SECTION;
alias gthread_recursive_mutex_t = CRITICAL_SECTION;

enum GTHREAD_MUTEX_INIT = CRITICAL_SECTION.init;
enum GTHREAD_ONCE_INIT  = gthread_once_t(0, -1);
enum GTHREAD_RECURSIVE_MUTEX_INIT = CRITICAL_SECTION.init;

version (MinGW)
{
  // Mingw runtime >= v0.3 provides a magic variable that is set to nonzero
  // if -mthreads option was specified, or 0 otherwise.
  extern(C)
  {
    extern __gshared int _CRT_MT;
    extern nothrow int __mingwthr_key_dtor(ULONG, void function(void*));
  }
}

// Backend thread functions
extern(C):
@trusted:
nothrow:

int gthread_active_p()
{
  version (MinGW)
    return _CRT_MT;
  else
    return 1;
}

int gthread_once(gthread_once_t* once, void function() nothrow func)
{
  if (! gthread_active_p())
    return -1;
  else if (once == null || func == null)
    return EINVAL;

  if (! once.done)
    {
      if (InterlockedIncrement(&(once.started)) == 0)
        {
          func();
          once.done = TRUE;
        }
      else
        {
          /* Another thread is currently executing the code, so wait for it
             to finish; yield the CPU in the meantime.  If performance
             does become an issue, the solution is to use an Event that
             we wait on here (and set above), but that implies a place to
             create the event before this routine is called.  */
          while (! once.done)
            Sleep(0);
        }
    }
  return 0;
}

/* Windows32 thread local keys don't support destructors; this leads to
   leaks, especially in threaded applications making extensive use of
   C++ EH. Mingw uses a thread-support DLL to work-around this problem.  */
int gthread_key_create(gthread_key_t* key, void function(void*) dtor)
{
  DWORD tlsindex = TlsAlloc();

  if (tlsindex != 0xFFFFFFFF)
    {
      *key = tlsindex;
      /* Mingw runtime will run the dtors in reverse order for each thread
         when the thread exits.  */
      version (MinGW)
        return __mingwthr_key_dtor(*key, dtor);
    }
  else
    return GetLastError();

  return 0;
}

int gthread_key_delete(gthread_key_t key)
{
  if (TlsFree(key) != 0)
    return 0;
  else
    return GetLastError();
}

void* gthread_getspecific(gthread_key_t key)
{
  DWORD lasterror = GetLastError();
  void* ptr = TlsGetValue(key);

  SetLastError(lasterror);

  return ptr;
}

int gthread_setspecific(gthread_key_t key, in void* ptr)
{
  if (TlsSetValue(key, cast(void*) ptr) != 0)
    return 0;
  else
    return GetLastError();
}

void gthread_mutex_init(gthread_mutex_t* mutex)
{
  InitializeCriticalSection(mutex);
}

int gthread_mutex_destroy(gthread_mutex_t* mutex)
{
  DeleteCriticalSection(mutex);
  return 0;
}

int gthread_mutex_lock(gthread_mutex_t* mutex)
{
  if (gthread_active_p())
    EnterCriticalSection(mutex);

  return 0;
}

int gthread_mutex_trylock(gthread_mutex_t* mutex)
{
  if (gthread_active_p())
    return TryEnterCriticalSection(mutex);
  else
    return 0;
}

int gthread_mutex_unlock(gthread_mutex_t* mutex)
{
  if (gthread_active_p())
    LeaveCriticalSection(mutex);

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

