/**
 * Implementation of support routines for synchronized blocks.
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
module rt.critical_;

private
{
    debug(PRINTF) import core.stdc.stdio;
    import core.stdc.stdlib;
    import gcc.gthreads;

    /* We don't initialize critical sections unless we actually need them.
     * So keep a linked list of the ones we do use, and in the static destructor
     * code, walk the list and release them.
     */
    struct critsec_t
    {
        critsec_t *next;
        gthread_recursive_mutex_t cs;
    }
}

/******************************************
 * Enter/exit critical section.
 */

static __gshared critsec_t *dcs_list;
static __gshared critsec_t critical_section;

extern (C) void _d_criticalenter(critsec_t *dcs)
{
  if (!dcs_list)
    {
      _STI_critical_init();
      atexit(&_STD_critical_term);
    }
  debug(PRINTF) printf("_d_criticalenter(dcs = x%x)\n", dcs);
  if (!dcs.next)
    {
      gthread_recursive_mutex_lock(&critical_section.cs);
      if (!dcs.next) // if, in the meantime, another thread didn't set it
	{
	  dcs.next = dcs_list;
	  dcs_list = dcs;
	  gthread_recursive_mutex_init(&dcs.cs);
	}
      gthread_recursive_mutex_unlock(&critical_section.cs);
    }
  gthread_recursive_mutex_lock(&dcs.cs);
}

extern (C) void _d_criticalexit(critsec_t *dcs)
{
  debug(PRINTF) printf("_d_criticalexit(dcs = x%x)\n", dcs);
  gthread_recursive_mutex_unlock(&dcs.cs);
}

extern (C) void _STI_critical_init()
{
  if (!dcs_list)
    {
      debug(PRINTF) printf("_STI_critical_init()\n");
      // The global critical section doesn't need to be recursive
      gthread_recursive_mutex_init(&critical_section.cs);
      dcs_list = &critical_section;
    }
}

extern (C) void _STD_critical_term()
{
  if (dcs_list)
    {
      debug(PRINTF) printf("_STI_critical_term()\n");
      while (dcs_list)
	{
	  debug(PRINTF) printf("\tlooping... %x\n", dcs_list);
	  gthread_recursive_mutex_destroy(&dcs_list.cs);
	  dcs_list = dcs_list.next;
	}
    }
}

