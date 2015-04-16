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

#ifndef LINUX_NAMESPACES_H
#define LINUX_NAMESPACES_H

/* Filenames of Linux namespaces in /proc/PID/ns.  */

/* IPC namespace: System V IPC, POSIX message queues.  */
#define LINUX_NS_IPC "ipc"

/* Mount namespace: mount points.  */
#define LINUX_NS_MNT "mnt"

/* Network namespace: network devices, stacks, ports, etc.  */
#define LINUX_NS_NET "net"

/* PID namespace: process IDs.  */
#define LINUX_NS_PID "pid"

/* User namespace: user and group IDs.  */
#define LINUX_NS_USER "user"

/* UTS namespace: hostname and NIS domain name.  */
#define LINUX_NS_UTS "uts"


/* Return nonzero if processes PID1 and PID2 have the same TYPE
   namespace, or if the kernel does not support TYPE namespaces
   (in which case there is only one TYPE namespace).  Return zero
   if the kernel supports TYPE namespaces and the two processes
   have different TYPE namespaces.  */

extern int linux_ns_same (int pid1, int pid2, const char *type);

/* Enter the TYPE namespace of process PID and call FUNC with the
   argument ARG, returning to the original TYPE namespace afterwards.
   If process PID has the same TYPE namespace as the current process,
   or if TYPE namespaces are not supported, just call FUNC with ARG.
   Return nonzero if FUNC was called, zero otherwise (and set ERRNO). */

extern int linux_ns_enter (int pid, const char *type,
			   void (*func) (void *), void *arg);

#endif /* LINUX_NAMESPACES_H */
