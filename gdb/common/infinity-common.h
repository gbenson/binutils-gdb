/* XXX To infinity... and beyond!

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

#ifndef COMMON_INFINITY_H
#define COMMON_INFINITY_H

/* Set to nonzero to enable debugging of GNU Infinity code.  */

extern int debug_infinity;

/* A GNU Infinity function.  */

struct infinity_function
{
  /* The raw data this function was created from.  */
  gdb_byte *data;
  size_t size;

  /* If nonzero, free_infinity_function should free DATA.  */
  int data_needs_free;

  /* The function's provider and name.  Together these form the
     identifier by which the function will be referenced.  Both
     point into DATA and do not need to be otherwise freed.  */
  gdb_byte *provider;
  gdb_byte *name;
};

/* Allocate a new struct infinity_function and populate it from NOTE.
   Returns non-NULL on success, NULL if NOTE could not be parsed.
   Non-NULL results must be freed with free_infinity_function.
   DATA will be freed by free_infinity_function if DATA_NEEDS_FREE
   is nonzero.  */

struct infinity_function *new_infinity_function (gdb_byte *data,
						 size_t size,
						 int data_needs_free);

/* Free an infinity function created by new_infinity_function.  */

void free_infinity_function (struct infinity_function *func);

/* Extract an unsigned integer of SIZE bytes from ADDR.  This
   function must be provided by the caller.  */

ULONGEST infinity_extract_uint (const gdb_byte *addr, int size);

#endif /* COMMON_INFINITY_H */
