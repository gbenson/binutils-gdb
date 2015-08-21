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

/* XXX.  */

enum infinity_skip_reason
  {
    NOTE_CORRUPT,
    NOTE_UNHANDLED,
    NOTE_UNUSABLE,
  };

/* XXX.  */

#define i8_skip_warning(reason, fmt, args...)		\
  do {							\
    i8_skip_warning_1 (__FILE__, __LINE__,		\
		       (reason), (fmt), ##args);	\
  } while (0);

/* XXX.  */

extern void i8_skip_warning_1 (const char *file, int line,
			       enum infinity_skip_reason reason,
			       const char *fmt, ...)
  ATTRIBUTE_PRINTF (4, 5);

/* Extract an unsigned integer of SIZE bytes from ADDR.  This
   function must be provided by the caller.  */

extern ULONGEST i8_extract_uint (const gdb_byte *addr, int size);

/* XXX.  */

extern const char *i8_extract_ident (const gdb_byte *ident,
				     const gdb_byte *limit);

/* XXX.  */

struct infinity_function
{
  /* XXX.  */
  gdb_byte *data;
  size_t size;

  /* XXX.  */
  const char *provider, *name;

  /* XXX.  */
  int num_args;
  int num_returns;

  /* XXX.  */
  int codesize;
  const gdb_byte *code;

  /* XXX.  */
  int num_symbols;
  gdb_byte *symbols;

  /* XXX.  */
  const gdb_byte *strings;
};

/* XXX.  */

extern struct infinity_function *i8func_init (const gdb_byte *data,
					      size_t size,
					      int addr_size);

/* XXX.  */

extern void i8func_free (struct infinity_function *func);

/* XXX.  */

extern const char *i8func_extract_ident (struct infinity_function *func,
					 const gdb_byte *offsetp);

#endif /* COMMON_INFINITY_H */
