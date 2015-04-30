/* Cleanup utilities for GDB, the GNU debugger.

   Copyright (C) 1986-2015 Free Software Foundation, Inc.

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
#include "cleanup-utils.h"

/* Helper function which does the work for make_cleanup_close.  */

static void
do_close_cleanup (void *arg)
{
  int *fd = arg;

  close (*fd);
}

/* See cleanup-utils.h.  */

struct cleanup *
make_cleanup_close (int fd)
{
  int *saved_fd = xmalloc (sizeof (fd));

  *saved_fd = fd;
  return make_cleanup_dtor (do_close_cleanup, saved_fd, xfree);
}
