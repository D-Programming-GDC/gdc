/**
 * Contains the implementation for object monitors.
 *
 * Copyright: Copyright Digital Mars 2000 - 2011.
 * License:   <a href="http://www.boost.org/LICENSE_1_0.txt">Boost License 1.0</a>.
 * Authors:   Walter Bright, Sean Kelly
 */

/*          Copyright Digital Mars 2000 - 2011.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

/* NOTE: This file has been patched from the original DMD distribution to
 * work with the GDC compiler.
 */
module rt.monitor_;

//debug=PRINTF;

nothrow:

private
{
    debug(PRINTF) import core.stdc.stdio;
    import core.stdc.stdlib;
    import gcc.gthreads;

    // This is what the monitor reference in Object points to
    alias Object.Monitor        IMonitor;
    alias void delegate(Object) DEvent;

    struct Monitor
    {
        IMonitor impl; // for user-level monitors
        DEvent[] devt; // for internal monitors
        size_t   refs; // reference count
        gthread_recursive_mutex_t mon;
    }

    Monitor* getMonitor(Object h) pure
    {
        return cast(Monitor*) h.__monitor;
    }

    void setMonitor(Object h, Monitor* m) pure
    {
        h.__monitor = m;
    }

    static __gshared int inited;
}


static __gshared gthread_recursive_mutex_t _monitor_critsec;

extern (C) void _STI_monitor_staticctor()
{
    debug(PRINTF) printf("+_STI_monitor_staticctor()\n");
    if (!inited)
    {
        gthread_recursive_mutex_init(&_monitor_critsec);
        inited = 1;
    }
    debug(PRINTF) printf("-_STI_monitor_staticctor()\n");
}

extern (C) void _STD_monitor_staticdtor()
{
    debug(PRINTF) printf("+_STI_monitor_staticdtor() - d\n");
    if (inited)
    {
        inited = 0;
        gthread_recursive_mutex_destroy(&_monitor_critsec);
    }
    debug(PRINTF) printf("-_STI_monitor_staticdtor() - d\n");
}

extern (C) void _d_monitor_create(Object h)
{
  /*
   * NOTE: Assume this is only called when h.__monitor is null prior to the
   * call.  However, please note that another thread may call this function
   * at the same time, so we can not assert this here.  Instead, try and
   * create a lock, and if one already exists then forget about it.
   */

  debug(PRINTF) printf("+_d_monitor_create(%p)\n", h);
  assert(h);
  Monitor *cs;
  gthread_recursive_mutex_lock(&_monitor_critsec);
  if (!h.__monitor)
    {
      cs = cast(Monitor *)calloc(Monitor.sizeof, 1);
      assert(cs);
      gthread_recursive_mutex_init(&cs.mon);
      setMonitor(h, cs);
      cs.refs = 1;
      cs = null;
    }
  gthread_recursive_mutex_unlock(&_monitor_critsec);
  if (cs)
    free(cs);
  debug(PRINTF) printf("-_d_monitor_create(%p)\n", h);
}

extern (C) void _d_monitor_destroy(Object h)
{
  debug(PRINTF) printf("+_d_monitor_destroy(%p)\n", h);
  assert(h && h.__monitor && !getMonitor(h).impl);
  gthread_recursive_mutex_destroy(&getMonitor(h).mon);
  free(h.__monitor);
  setMonitor(h, null);
  debug(PRINTF) printf("-_d_monitor_destroy(%p)\n", h);
}

extern (C) void _d_monitor_lock(Object h)
{
  debug(PRINTF) printf("+_d_monitor_acquire(%p)\n", h);
  assert(h && h.__monitor && !getMonitor(h).impl);
  gthread_recursive_mutex_lock(&getMonitor(h).mon);
  debug(PRINTF) printf("-_d_monitor_acquire(%p)\n", h);
}

extern (C) void _d_monitor_unlock(Object h)
{
  debug(PRINTF) printf("+_d_monitor_release(%p)\n", h);
  assert(h && h.__monitor && !getMonitor(h).impl);
  gthread_recursive_mutex_unlock(&getMonitor(h).mon);
  debug(PRINTF) printf("-_d_monitor_release(%p)\n", h);
}

