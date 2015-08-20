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

#include "defs.h"
#include "infinity.h"
#include "objfiles.h"
#include "observer.h"

/* Called whenever a new object file is loaded.  */

static void
infinity_new_objfile (struct objfile *objfile)
{
  if (objfile == NULL)
    return;

  debug_printf ("\x1B[32m%s: %s\x1B[0m\n", __FUNCTION__,
		objfile_name (objfile));
}

/* Called whenever an object file is unloaded.  */

static void
infinity_free_objfile (struct objfile *objfile)
{
  debug_printf ("\x1B[31m%s: %s\x1B[0m\n", __FUNCTION__,
		objfile_name (objfile));
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_infinity;

void
_initialize_infinity (void)
{
  /* Notice when object files get loaded and unloaded.  */
  observer_attach_new_objfile (infinity_new_objfile);
  observer_attach_free_objfile (infinity_free_objfile);
}
