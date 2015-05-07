/* XXX Infinity!

   Copyright (C) 2015 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "infinity.h"
#include <thread_db.h>

#define NotImplemented() \
  error ("\x1B[1;41;33m%s: UNIMPLEMENTED\x1B[0m", __FUNCTION__)

static td_err_e
trace (td_err_e result, const char *fmt, ...)
{
  va_list ap;

  debug_printf ("\x1B[%dm", result == TD_OK ? 32 : 31);

  va_start (ap, fmt);
  debug_vprintf (fmt, ap);
  va_end (ap);

  if (result != TD_OK)
    debug_printf (" => %d (%s)", result, thread_db_err_str (result));

  debug_printf ("\x1B[0m\n");

  return result;
}

/* In glibc this does nothing.  */

td_err_e
infinity_td_init (void)
{
  return trace (td_init (), "td_init ()");
}

/* In glibc this allocates some memory to store TA, and checks
   version.  It's looking for the symbol "nptl_version" to be an
   exact match of the string VERSION, defined in src/version.h,
   which on my RHEL6 box is "2.12".  td_thragent_t is opaque.  */

td_err_e
infinity_td_ta_new (struct ps_prochandle *ps, td_thragent_t **ta)
{
  td_err_e result = trace (td_ta_new (ps, ta), "td_ta_new (ps, ta)");

  if (result == TD_OK)
    debug_printf ("\x1B[32m  ta <= %p\x1B[0m\n", *ta);

  return result;
}

td_err_e
infinity_td_ta_map_id2thr (const td_thragent_t *ta, pthread_t pt,
			   td_thrhandle_t *th)
{
  NotImplemented ();
}

/* In glibc this is some hairy machine-dependent stuff.
   td_thrhandle_t is NOT OPAQUE to GDB!  */

td_err_e
infinity_td_ta_map_lwp2thr (const td_thragent_t *ta, lwpid_t lwpid,
			    td_thrhandle_t *th)
{
  return trace (td_ta_map_lwp2thr (ta, lwpid, th),
		"td_ta_map_lwp2thr (%p, %d, %p)", ta, lwpid, th);
}

td_err_e
infinity_td_ta_thr_iter (const td_thragent_t *ta,
			 td_thr_iter_f *callback, void *cbdata_p,
			 td_thr_state_e state, int ti_pri,
			 sigset_t *ti_sigmask_p,
			 unsigned int ti_user_flags)
{
  NotImplemented ();
}

td_err_e
infinity_td_thr_validate (const td_thrhandle_t *th)
{
  NotImplemented ();
}

td_err_e
infinity_td_thr_get_info (const td_thrhandle_t *th, td_thrinfo_t *infop)
{
  return trace (td_thr_get_info (th, infop),
		"td_thr_get_info (%p, %p)", th, infop);
}

td_err_e
infinity_td_ta_event_addr (const td_thragent_t *ta, td_event_e event,
			   td_notify_t *ptr)
{
  /* Checked by enable_thread_event_reporting,
     not required if the kernel supports clone events,
     marked as "not essential" in try_thread_db_load_2.  */
  NotImplemented ();
}

td_err_e
infinity_td_ta_set_event (const td_thragent_t *ta,
			  td_thr_events_t *event)
{
  /* Checked by enable_thread_event_reporting,
     not required if the kernel supports clone events,
     marked as "not essential" in try_thread_db_load_2.  */
  NotImplemented ();
}

td_err_e
infinity_td_ta_clear_event (const td_thragent_t *ta,
			    td_thr_events_t *event)
{
  /* Checked by disable_thread_event_reporting,
     not required if the kernel supports clone events,
     marked as "not essential" in try_thread_db_load_2.  */
  NotImplemented ();
}

td_err_e
infinity_td_ta_event_getmsg (const td_thragent_t *ta,
			     td_event_msg_t *msg)
{
  /* Checked by enable_thread_event_reporting,
     not required if the kernel supports clone events,
     marked as "not essential" in try_thread_db_load_2.  */
  NotImplemented ();
}

td_err_e
infinity_td_thr_event_enable (const td_thrhandle_t *th, int event)
{
  /* Checked by enable_thread_event_reporting,
     not required if the kernel supports clone events,
     marked as "not essential" in try_thread_db_load_2.  */
  NotImplemented ();
}

td_err_e
infinity_td_thr_tls_get_addr (const td_thrhandle_t *th,
			      psaddr_t map_address, size_t offset,
			      psaddr_t *address)
{
  /* Not required for initial attach, but required for "p errno".  */
  return trace (td_thr_tls_get_addr (th, map_address, offset, address),
		"td_thr_tls_get_addr (%p, %p, %ld, %p)",
		th, map_address, offset, address);
}

td_err_e
infinity_td_thr_tlsbase (const td_thrhandle_t *th,
			 unsigned long int modid, psaddr_t *base)
{
  /* Not required for initial attach, but required for "p errno"
     for -static -pthread executables.  Damn edge cases...  */
  return trace (td_thr_tlsbase (th, modid, base),
		"td_thr_tlsbase (%p, %ld, %p)", th, modid, base);
}
