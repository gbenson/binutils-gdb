/* Declarations for common target functions.

   Copyright (C) 1986-2014 Free Software Foundation, Inc.

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

#ifndef TARGET_COMMON_H
#define TARGET_COMMON_H

/* This header is a stopgap until more code is shared.  */

/* Resume execution of the target process PTID (or a group of
   threads).  STEP says whether to single-step or to run free; SIGGNAL
   is the signal to be given to the target, or GDB_SIGNAL_0 for no
   signal.  The caller may not pass GDB_SIGNAL_DEFAULT.  A specific
   PTID means `step/resume only this process id'.  A wildcard PTID
   (all threads, or all threads of process) means `step/resume
   INFERIOR_PTID, and let other threads (for which the wildcard PTID
   matches) resume with their 'thread->suspend.stop_signal' signal
   (usually GDB_SIGNAL_0) if it is in "pass" state, or with no signal
   if in "no pass" state.  */

extern void target_resume (ptid_t ptid, int step, enum gdb_signal signal);

/* Wait for process pid to do something.  PTID = -1 to wait for any
   pid to do something.  Return pid of child, or -1 in case of error;
   store status through argument pointer STATUS.  Note that it is
   _NOT_ OK to throw_exception() out of target_wait() without popping
   the debugging target from the stack; GDB isn't prepared to get back
   to the prompt with a debugging target but without the frame cache,
   stop_pc, etc., set up.  OPTIONS is a bitwise OR of TARGET_W*
   options.  */

extern ptid_t target_wait (ptid_t ptid, struct target_waitstatus *status,
			   int options);

/* Make target stop in a continuable fashion.  (For instance, under
   Unix, this should act like SIGSTOP).  This function is normally
   used by GUIs to implement a stop button.  */

extern void target_stop (ptid_t ptid);

/* Read LEN bytes of target memory at address MEMADDR, placing the
   results in GDB's memory at MYADDR.  Return zero for success,
   nonzero if any error occurs.  Implementations of this function may
   define and use their own error codes, but functions in the common,
   nat and target directories must treat the return code as opaque.
   No guarantee is made about the contents of the data at MYADDR if
   any error occurs.  */

extern int target_read_memory (CORE_ADDR memaddr, gdb_byte *myaddr,
			       ssize_t len);

/* Read an unsigned 32-bit integer in the target's format from target
   memory at address MEMADDR, storing the result in GDB's format in
   GDB's memory at RESULT.  Return zero for success, nonzero if any
   error occurs.  Implementations of this function may define and use
   their own error codes, but functions in the common, nat and target
   directories must treat the return code as opaque.  No guarantee is
   made about the contents of the data at RESULT if any error
   occurs.  */

extern int target_read_uint32 (CORE_ADDR memaddr, unsigned int *result);

/* Write LEN bytes from MYADDR to target memory at address MEMADDR.
   Return zero for success, nonzero if any error occurs.
   Implementations of this function may define and use their own error
   codes, but functions in the common, nat and target directories must
   treat the return code as opaque.  No guarantee is made about the
   contents of the data at MEMADDR if any error occurs.  */

extern int target_write_memory (CORE_ADDR memaddr, const gdb_byte *myaddr,
				ssize_t len);

/* If set, the inferior should be controlled in non-stop mode.  In
   this mode, each thread is controlled independently.  Execution
   commands apply only to the selected thread by default, and stop
   events stop only the thread that had the event -- the other threads
   are kept running freely.  */

extern int non_stop;

#endif /* TARGET_COMMON_H */
