/* Linux namespaces(7) support.

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

#include "common-defs.h"
#include "nat/linux-namespaces.h"
#include "filestuff.h"
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sched.h>

/* XXX!

   "A process may not be reassociated with a new mount namespace if it
   is multithreaded."
     -- http://man7.org/linux/man-pages/man2/setns.2.html

*/

/* Handle systems with __NR_setns but no setns.  */
#if defined(__NR_setns) && !defined(HAVE_SETNS)
static int
setns (int fd, int nstype)
{
  return syscall (__NR_setns, fd, nstype);
}
#define HAVE_SETNS 1
#endif /* defined(__NR_setns) && !defined(HAVE_SETNS) */

#ifdef HAVE_SETNS

/* Return the path of the TYPE namespace entry for process PID.
   The returned value persists until this function is next called.  */

static const char *
linux_ns_path_for (int pid, const char *type)
{
  static char path[PATH_MAX];

  xsnprintf (path, sizeof (path), "/proc/%d/ns/%s", pid, type);

  return path;
}

/* See nat/linux-namespaces.h.  */

int
linux_ns_same (int pid1, int pid2, const char *type)
{
  const char *path;
  struct stat sb;
  ino_t pid1_id;

  if (pid1 == pid2)
    return 1;

  /* Lack of kernel support for TYPE namespaces is detected by
     attempting to open our own.  Try to make PID1 be our own
     PID so we don't have to do extra checking if the the first
     stat fails.  */
  if (pid2 == getpid ())
    {
      int swap = pid1;

      pid1 = pid2;
      pid2 = swap;
    }

  path = linux_ns_path_for (pid1, type);
  if (stat (path, &sb) != 0)
    {
      int saved_errno;

      if (pid1 == getpid ())
	{
	  /* Assume the kernel does not support TYPE namespaces.  */
	  return 1;
	}

      saved_errno = errno;
      path = linux_ns_path_for (getpid (), type);
      if (stat (path, &sb) != 0)
	{
	  /* Assume the kernel does not support TYPE namespaces.  */
	  return 1;
	}

      /* We can open our own TYPE namespace but not that for process
	 PID.  The process might have died, or we might not have the
	 right permissions (though we should be attached by this time
	 so this seems unlikely).  In any event, we cannot make any
	 decisions and must throw.  */
      errno = saved_errno;
      perror_with_name (linux_ns_path_for (pid1, type));
    }
  pid1_id = sb.st_ino;

  /* The kernel definitely supports TYPE namespaces so we cannot
     make any decisions if this stat fails.  */
  path = linux_ns_path_for (pid2, type);
  if (stat (path, &sb) != 0)
    perror_with_name (path);

  return sb.st_ino == pid1_id;
}

/* Helper function which does the work for make_cleanup_setns.  */

static void
do_setns_cleanup (void *arg)
{
  int *fd = arg;

  if (setns (*fd, 0) != 0)
    internal_error (__FILE__, __LINE__,
		    _("unable to restore namespace: %s"),
		    safe_strerror (errno));
}

/* Return a new cleanup that calls setns on FD.  */

static struct cleanup *
make_cleanup_setns (int fd)
{
  int *saved_fd = xmalloc (sizeof (fd));

  *saved_fd = fd;
  return make_cleanup_dtor (do_setns_cleanup, saved_fd, xfree);
}


/* See nat/linux-namespaces.h.  */

int
linux_ns_enter (int pid, const char *type,
		void (*func) (void *), void *arg)
{
  struct cleanup *old_chain, *close_pidfd;
  int our_fd, pid_fd;
  struct stat sb;
  ino_t our_id;

  /* Open our TYPE namespace.  On failure, assume the kernel does
     not support TYPE namespaces (so every process is in the same
     TYPE namespace) and just call FUNC with ARG and return.  */
  our_fd = gdb_open_cloexec (linux_ns_path_for (getpid(), type),
			     O_RDONLY, 0);
  if (our_fd < 0)
    {
      func (arg);
      return 1;
    }
  old_chain = make_cleanup_close (our_fd);

  /* We've successfully opened our own TYPE namespace.  The kernel
     obviously supports TYPE namespaces and any subsequent errors
     cannot be continued over.  */

  if (fstat (our_fd, &sb) != 0)
    {
      do_cleanups (old_chain);
      return 0;
    }
  our_id = sb.st_ino;

  pid_fd = gdb_open_cloexec (linux_ns_path_for (pid, type),
			     O_RDONLY, 0);
  if (pid_fd < 0)
    {
      do_cleanups (old_chain);
      return 0;
    }
  close_pidfd = make_cleanup_close (pid_fd);

  if (fstat (pid_fd, &sb) != 0)
    {
      do_cleanups (old_chain);
      return 0;
    }

  if (sb.st_ino == our_id)
    {
      /* The other process has the same TYPE namespace as us.  */
      do_cleanups (old_chain);
      func (arg);
      return 1;
    }

  if (setns (pid_fd, 0) != 0)
    {
      do_cleanups (old_chain);
      return 0;
    }
  do_cleanups (close_pidfd);
  make_cleanup_setns (our_fd);

  func (arg);

  do_cleanups (old_chain);
  return 1;
}

#else /* HAVE_SETNS */

/* See nat/linux-namespaces.h.  */

int
linux_ns_same (int pid1, int pid2, const char *type)
{
  return 1;
}

/* See nat/linux-namespaces.h.  */

int
linux_ns_enter (int pid, const char *type,
		void (*func) (void *), void *arg)
{
  func (arg);

  return 1;
}

#endif  /* HAVE_SETNS */
