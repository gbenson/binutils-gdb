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

#include "common-defs.h"
#include "infinity-common.h"

/* See infinity-common.h.  */
int debug_infinity;

/* See infinity-common.h.  */

struct infinity_function *
new_infinity_function (gdb_byte *data, size_t size, int data_needs_free)
{
  struct infinity_function *func;
  int version, reserved1, reserved2;
  gdb_byte *provider, *name, *p, *pl = data + size;

  /* Ensure the note is not too small.  18 bytes is 16 header bytes,
     1 byte for the NUL of provider and 1 byte for the NUL of name.  */
  if (size < 18)
    return NULL;

  /* Check the version and reserved fields.  */
  version = infinity_extract_uint (data, 2);
  if (version != 1)
    return NULL;

  reserved1 = infinity_extract_uint (data + 2, 2);
  if (reserved1 != 0)
    return NULL;

  reserved2 = infinity_extract_uint (data + 12, 4);
  if (reserved1 != 0)
    return NULL;

  /* Extract the name and provider.  */
  provider = p = data + 16;
  while (p < pl && *p != '\0')
    p++;
  name = p += 1;
  while (p < pl && *p != '\0')
    p++;
  if (p >= pl)
    return NULL;

  func = XCNEW (struct infinity_function);
  func->data = data;
  func->size = size;
  func->data_needs_free = data_needs_free;
  func->provider = provider;
  func->name = name;

  return func;
}

/* See infinity-common.h.  */

void
free_infinity_function (struct infinity_function *func)
{
  if (func->data_needs_free)
    xfree (func->data);

  xfree (func);
}
