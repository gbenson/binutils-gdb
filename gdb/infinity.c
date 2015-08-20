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
#include "elf-bfd.h"
#include "objfiles.h"
#include "observer.h"

/* Per-program-space data.  */
struct infinity_context
{
};

/* Per-program-space data key.  */
static const struct program_space_data *infinity_pspace_data;

/* Get the per-program-space data.  If none is found, allocate and
   initialize one.  This function always returns a valid object.  */

static struct infinity_context *
get_infinity_context (void)
{
  struct infinity_context *ctx;

  ctx = program_space_data (current_program_space, infinity_pspace_data);
  if (ctx != NULL)
    return ctx;

  ctx = XCNEW (struct infinity_context);
  set_program_space_data (current_program_space, infinity_pspace_data, ctx);

  debug_printf ("\x1B[32m%s: created %p\x1B[0m\n", __FUNCTION__, ctx);

  return ctx;
}

/* Free the per-program-space data.  */

static void
infinity_context_cleanup (struct program_space *pspace, void *arg)
{
  struct infinity_context *ctx = arg;

  debug_printf ("\x1B[32m%s: freeing %p\x1B[0m\n", __FUNCTION__, ctx);

  xfree (ctx);
}

/* Called whenever a new note is loaded.  */

static void
infinity_new_note (struct infinity_note *note)
{
  struct infinity_context *ctx = get_infinity_context ();

  debug_printf ("\x1B[32m%s: size = %ld\x1B[0m\n", __FUNCTION__, note->size);
}

/* Called whenever a note is unloaded.  */

static void
infinity_free_note (struct infinity_note *note)
{
  struct infinity_context *ctx = get_infinity_context ();

  debug_printf ("\x1B[32m%s: size = %ld\x1B[0m\n", __FUNCTION__, note->size);
}

/* Call FUNC for each GNU Infinity note in OBJFILE.  */

static void
foreach_infinity_note (struct objfile *objfile,
		       void (*func) (struct infinity_note *))
{
  struct infinity_note *note;

  if (objfile == NULL
      || objfile->obfd == NULL
      || bfd_get_flavour (objfile->obfd) != bfd_target_elf_flavour)
    return;

  for (note = elf_tdata (objfile->obfd)->infinity_note_head;
       note != NULL; note = note->next)
    func (note);
}

/* Called whenever a new object file is loaded.  */

static void
infinity_new_objfile (struct objfile *objfile)
{
  foreach_infinity_note (objfile, infinity_new_note);
}

/* Called whenever an object file is unloaded.  */

static void
infinity_free_objfile (struct objfile *objfile)
{
  foreach_infinity_note (objfile, infinity_free_note);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_infinity;

void
_initialize_infinity (void)
{
  /* Register our per-program-space data.  */
  infinity_pspace_data
    = register_program_space_data_with_cleanup (NULL,
						infinity_context_cleanup);

  /* Notice when object files get loaded and unloaded.  */
  observer_attach_new_objfile (infinity_new_objfile);
  observer_attach_free_objfile (infinity_free_objfile);
}
