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
#include "common-infinity.h"
#include <ctype.h>

/* See infinity-common.h.  */

int debug_infinity;

/* See infinity-common.h.  */

void i8_skip_warning_1 (const char *file, int line,
			enum infinity_skip_reason reason,
			const char *fmt, ...)
{
  switch (reason)
    {
    case NOTE_CORRUPT:
      warning (_("Skipping corrupt GNU Infinity note."));
      break;

    case NOTE_UNHANDLED:
      warning (_("Skipping unhandled GNU Infinity note."));
      break;

    case NOTE_UNUSABLE:
      warning (_("Skipping unusable GNU Infinity note."));
      break;

    default:
      gdb_assert_not_reached (_("bad switch"));
    }

  if (debug_infinity)
    {
      va_list ap;

      debug_printf ("%s:%d: ", file, line);
      debug_printf ("\x1B[1;31m"); // XXX
      va_start (ap, fmt);
      debug_vprintf (fmt, ap);
      va_end (ap);
      debug_printf ("\x1B[0m"); // XXX
      debug_printf ("\n");
    }
}

/* See infinity-common.h.  */

const char *
i8_extract_ident (const gdb_byte *ident, const gdb_byte *limit)
{
  const gdb_byte *c = ident;

  if (*c == '\0')
    {
      i8_skip_warning (NOTE_CORRUPT, "zero-length identifier");
      return NULL;
    }
  if (isdigit (*c))
    {
      i8_skip_warning (NOTE_CORRUPT, "identifier starts with digit");
      return NULL;
    }

  for (; c < limit; c++)
    {
      if (*c == '\0')
	return (const char *) ident;

      if (*c == '_' || isalnum (*c))
	continue;

      i8_skip_warning (NOTE_CORRUPT, "0x%2x in identifier", *c);
      return NULL;
    }

  i8_skip_warning (NOTE_CORRUPT, "unterminated identifier");
  return NULL;
}

/* Cleanup version of i8func_free.  */

static void
i8func_free_cleanup (void *funcp)
{
  i8func_free ((struct infinity_function *) funcp);
}

/* See infinity-common.h.  */

struct infinity_function *
i8func_init (const gdb_byte *data, size_t size, int addr_size)
{
  struct infinity_function *func;
  const int min_hdrsize = 14;
  int version, hdrsize;
  struct cleanup *old_chain;
  gdb_byte *tmp;

  /* Read the version and header size.  */
  if (size < 6)
    {
      i8_skip_warning (NOTE_CORRUPT, "size = %s", pulongest (size));
      return NULL;
    }
  version = i8_extract_uint (data + 2, 2);
  if (version != 1)
    {
      i8_skip_warning (NOTE_UNHANDLED, "version = %d", version);
      return NULL;
    }
  hdrsize = i8_extract_uint (data + 4, 2);
  if (hdrsize < min_hdrsize || size < 4 + hdrsize)
    {
      i8_skip_warning (NOTE_CORRUPT, "hdrsize = %d", hdrsize);
      return NULL;
    }

  /* Create the function and copy the data into it.  */
  func = XCNEW (struct infinity_function);
  old_chain = make_cleanup (i8func_free_cleanup, func);
  func->data = xmalloc (size);
  func->size = size;
  memcpy (func->data, data, size);

  /* Extract the details.  */
  func->codesize = i8_extract_uint (data + 6, 2);
  func->num_symbols = i8_extract_uint (data + 12, 2);
  func->num_args = i8_extract_uint (data + 14, 2);
  func->num_returns = i8_extract_uint (data + 16, 2);

  /* Work out where everything is.  */
  func->code = tmp = func->data + 4 + align_up (hdrsize, 4);
  func->symbols = tmp + align_up (func->codesize, 4);
  func->strings = func->symbols + func->num_symbols * addr_size;

  /* Extract the provider and name.  This implicitly checks that
     the note is large enough to contain the code and symbols.  */
  func->provider = i8func_extract_ident (func, data + 8);
  if (func->provider == NULL)
    {
      do_cleanups (old_chain);
      return NULL;
    }

  func->name = i8func_extract_ident (func, data + 10);
  if (func->name == NULL)
    {
      do_cleanups (old_chain);
      return NULL;
    }

  discard_cleanups (old_chain);
  return func;
}

/* See infinity-common.h.  */

void
i8func_free (struct infinity_function *func)
{
  if (func != NULL)
    xfree (func->data);

  xfree (func);
}

/* See infinity-common.h.  */

const char *
i8func_extract_ident (struct infinity_function *func,
		      const gdb_byte *offsetp)
{
  return i8_extract_ident (func->strings + i8_extract_uint (offsetp, 2),
			   func->data + func->size);
}
