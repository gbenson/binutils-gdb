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

/* Cleanup utilities.  These are not declared in cleanups.h
   (nor defined in cleanups.c) because, while they use the
   cleanup API, they are not part of the cleanup API.  */

#ifndef CLEANUP_UTILS_H
#define CLEANUP_UTILS_H

/* Return a new cleanup that closes FD.  */

extern struct cleanup *make_cleanup_close (int fd);

#endif /* CLEANUP_UTILS_H */
