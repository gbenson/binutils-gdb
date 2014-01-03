/* Declarations of target symbol functions.

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

#ifndef TARGET_SYMBOL_H
#define TARGET_SYMBOL_H

struct objfile;

/* Find a symbol that matches NAME.  Limit the search to OBJFILE if
   OBJFILE is non-NULL and the implementation supports limiting the
   search to specific object files.  If a match is found, store the
   matching symbol's address in ADDR and return nonzero.  Return zero
   if no symbol matching NAME is found.  Raise an exception if OBJFILE
   is non-NULL and the implementation does not support limiting
   searches to specific object files.  */

extern int target_look_up_symbol (const char *name, CORE_ADDR *addr,
				  struct objfile *objfile);

#endif /* TARGET_SYMBOL_H */
