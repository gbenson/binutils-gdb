/* GDB routines for manipulating GNU Infinity notes.

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

/* Everything in this file is ultimately concerned with consuming
   objfile observer notifications and emitting infinity_function
   observer notifications.  */

#include "defs.h"
#include "common-infinity.h"
#include "elf-bfd.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "observer.h"

/* See infinity-common.h.  */

ULONGEST
i8_extract_uint (const gdb_byte *addr, int size)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());

  return extract_unsigned_integer (addr, size, byte_order);
}

/* XXX.  */

static void
i8_store_uint (gdb_byte *addr, int size, ULONGEST value)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());

  store_unsigned_integer (addr, size, byte_order, value);
}

/* XXX.  */

static char *
i8_make_fullname (const char *provider, const char *name)
{
  return concat (provider, "::", name, NULL);
}

/* Per-program-space data.  */

struct infinity_pspace_data
{
  /* XXX.  */
  htab_t by_elfnote;

  /* XXX.  */
  htab_t by_fullname;

  /* XXX.  */
  int needs_rebuild;
};

/* Forward declarations.  */
static struct infinity_pspace_data *
  i8psd_get (struct program_space *pspace);
static struct infinity_note *
  i8psd_get_note_by_fullname (struct infinity_pspace_data *psd,
			      const char *fullname);

/* XXX.  */

struct infinity_note
{
  /* XXX.  */
  struct infinity_note *next;

  /* XXX.  */
  struct elf_infinity_note *elfnote;

  /* XXX.  */
  struct objfile *objfile;

  /* XXX.  */
  const char *fullname;

  /* XXX.  */
  const struct infinity_note_ops *ops;
};

/* XXX.  */

struct infinity_note_ops
{
  /* XXX.  */
  const char *op_typename;

  /* XXX.  */
  size_t op_size;

  /* XXX.  */
  int (*op_init) (struct infinity_note *note);

  /* XXX.  */
  void (*op_free) (struct infinity_note *note);

  /* XXX.  */
  void (*op_relocate) (struct infinity_note *note,
		       struct infinity_pspace_data *psd);

  /* XXX.  */
  void (*op_rebuild) (struct infinity_note *note,
		      struct infinity_pspace_data *psd);

  /* XXX.  */
  void (*op_pre_remove) (struct infinity_note *note);
};

/* Note operations.  */

#define i8note_alloc(ops) \
  ((struct infinity_note *) xcalloc (1, (ops)->op_size))

#define i8note_init(note) \
  ((note)->ops->op_init ((note)))

#define i8note_relocate(note, psd) \
  do { \
    if ((note)->ops->op_relocate != NULL) \
      (note)->ops->op_relocate ((note), (psd)); \
  } while (0)

#define i8note_rebuild(note, psd) \
  do { \
    if ((note)->ops->op_rebuild != NULL) \
      (note)->ops->op_rebuild ((note), (psd)); \
  } while (0)

#define i8note_prepare_for_removal(note) \
  do { \
    if ((note)->ops->op_pre_remove != NULL) \
      (note)->ops->op_pre_remove ((note)); \
  } while (0)

/* XXX.  */

static void
i8note_free (void *notep)
{
  struct infinity_note *note = (struct infinity_note *) notep;

  if (note->ops->op_free != NULL)
    note->ops->op_free (note);

  xfree (note);
}

/* XXX.  */

static int
i8note_addr_size (const struct infinity_note *note)
{
  return gdbarch_addr_bit (get_objfile_arch (note->objfile)) / 8;
}

/* XXX.  */

static hashval_t
i8note_hash_elfnote (const void *notep)
{
  const struct infinity_note *note
    = (const struct infinity_note *) notep;

  return htab_hash_pointer (note->elfnote);
}

/* XXX.  */

static int
i8note_equal_elfnote (const void *note1p, const void *note2p)
{
  const struct infinity_note *note1
    = (const struct infinity_note *) note1p;
  const struct infinity_note *note2
    = (const struct infinity_note *) note2p;

  return note1->elfnote == note2->elfnote;
}

/* XXX.  */

static hashval_t
i8note_hash_fullname (const void *notep)
{
  const struct infinity_note *note
    = (const struct infinity_note *) notep;

  return htab_hash_string (note->fullname);
}

/* XXX.  */

static int
i8note_equal_fullname (const void *note1p, const void *note2p)
{
  const struct infinity_note *note1
    = (const struct infinity_note *) note1p;
  const struct infinity_note *note2
    = (const struct infinity_note *) note2p;

  return strcmp (note1->fullname, note2->fullname) == 0;
}

/* XXX.  */

struct symbol_note
{
  /* XXX.  */
  struct infinity_note note;

  /* XXX.  */
  CORE_ADDR note_address;

  /* XXX.  */
  int section_index;

  /* XXX.  */
  int is_resolved;

  /* XXX.  */
  CORE_ADDR loaded_address;
};

/* Argument for symbol_note_get_section.  */

struct symbol_note_get_section_arg
{
  /* The address we are trying to find containing sections for.  */
  CORE_ADDR address;

  /* The number of containing sections found.  */
  int count;

  /* The last containing section found.  */
  asection *section;
};

/* Find the section that ARG->address is in.  This function is called
   via bfd_map_over_sections.  ARG->count will be set to the number of
   sections that contain address, and ARG->section will be set to the
   last section that contained ARG->address.  */

static void
symbol_note_get_section (bfd *abfd, asection *sect, void *argp)
{
  struct symbol_note_get_section_arg *arg
    = (struct symbol_note_get_section_arg *) argp;
  flagword flags = bfd_get_section_flags (abfd, sect);
  bfd_vma start, limit;

  if ((flags & (SEC_ALLOC | SEC_LOAD)) == 0)
    return;

  start = bfd_section_vma (abfd, sect);
  if (arg->address < start)
    return;

  limit = start + bfd_section_size (abfd, sect);
  if (arg->address >= limit)
    return;

  arg->section = sect;
  arg->count++;
}

/* Implementation of infinity_note_ops->op_init.  */

static int
symbol_note_init (struct infinity_note *note)
{
  struct symbol_note *sym = (struct symbol_note *) note;
  const gdb_byte *ptr = note->elfnote->data;
  const gdb_byte *limit = ptr + note->elfnote->size;
  int addr_size = i8note_addr_size (note);
  const char *provider, *name;
  int version, skip;
  struct symbol_note_get_section_arg gsa;

  /* Unpack the symbol.  */
  gdb_assert (note->elfnote->size >= 4);
  ptr += 2; /* The note type has already been handled.  */

  version = i8_extract_uint (ptr, 2);
  if (version != 1)
    {
      i8_skip_warning (NOTE_UNHANDLED, "version = %d", version);
      return 0;
    }
  ptr += 2;

  if (ptr + addr_size >= limit)
    {
      i8_skip_warning (NOTE_CORRUPT, "size = %s",
		       pulongest (note->elfnote->size));
      return 0;
    }
  sym->note_address = i8_extract_uint (ptr, addr_size);
  ptr += addr_size;

  provider = i8_extract_ident (ptr, limit);
  if (provider == NULL)
    return 0;
  ptr += align_up (strlen (provider) + 1, 4);

  name = i8_extract_ident (ptr, limit);
  if (name == NULL)
    return 0;

  note->fullname = i8_make_fullname (provider, name);

  /* XXX.  */
  gsa.address = sym->note_address;
  gsa.count = 0;
  bfd_map_over_sections (note->objfile->obfd,
			 symbol_note_get_section, &gsa);

  if (gsa.count == 0)
    {
      i8_skip_warning (NOTE_UNUSABLE,
		       "%s (0x%s) not found in any section",
		       note->fullname,
		       phex_nz (sym->note_address,
				i8note_addr_size (note)));
      return 0;
    }
  else if (gsa.count > 1)
    {
      i8_skip_warning (NOTE_UNUSABLE,
		       "%s (0x%s) found in multiple sections",
		       note->fullname,
		       phex_nz (sym->note_address,
				i8note_addr_size (note)));
      return 0;
    }

  sym->section_index = gsa.section->index;

  return 1;
}

/* Implementation of infinity_note_ops->op_relocate.  */

static void
symbol_note_relocate (struct infinity_note *note,
		      struct infinity_pspace_data *psd)
{
  struct symbol_note *sym = (struct symbol_note *) note;

  sym->is_resolved = 0;
  psd->needs_rebuild = 1;
}

/* Operations for symbol notes.  */

static const struct infinity_note_ops symbol_note_ops =
{
  "symbol",				/* op_typename.  */
  sizeof (struct symbol_note),		/* op_size.  */
  symbol_note_init,			/* op_init.  */
  NULL,					/* op_free.  */
  symbol_note_relocate,			/* op_relocate.  */
  NULL,					/* op_rebuild.  */
  NULL,					/* op_pre_remove.  */
};

/* XXX.  */

#define i8note_as_symbol(note) \
  ((note)->ops == &symbol_note_ops \
   ? (struct symbol_note *) (note) : NULL)

/* XXX.  */

static CORE_ADDR
symbol_note_address (struct symbol_note *sym)
{
  if (!sym->is_resolved)
    {
      sym->loaded_address = sym->note_address
	+ ANOFFSET (sym->note.objfile->section_offsets,
		    sym->section_index);

      if (debug_infinity)
	{
	  struct bound_minimal_symbol msym;
	  int addr_size = i8note_addr_size (&sym->note);
	  const char *name;

	  debug_printf ("infinity: resolved symbol %s (0x%s",
			sym->note.fullname,
			phex_nz (sym->loaded_address, addr_size));

	  /* For notes created from C files using the INFINITY_EXPORT
	     macro there should be a regular symbol with the same name
	     if we have enough debuginfo.  See if we can find one, and
	     check the value if we do.  Mismatches aren't necessarily
	     errors, but they're worth investigating.  */
	  name = strstr (sym->note.fullname, "::");
	  gdb_assert (name != NULL);
	  name += 2;
	  msym = lookup_minimal_symbol (name, NULL, sym->note.objfile);
	  if (msym.minsym == NULL)
	    debug_printf (", unchecked");
	  else
	    {
	      CORE_ADDR check_addr = BMSYMBOL_VALUE_ADDRESS (msym);

	      if (sym->loaded_address != check_addr)
		debug_printf (" != 0x%s",
			      phex_nz (check_addr, addr_size));
	    }

	  debug_printf (")\n");
	}

      sym->is_resolved = 1;
    }

  return sym->loaded_address;
}

/* XXX.  */

struct symbol_reference
{
  /* XXX.  */
  char *fullname;

  /* XXX.  */
  gdb_byte *slot;
};

/* XXX.  */

struct function_note
{
  /* XXX.  */
  struct infinity_note note;

  /* XXX.  */
  struct infinity_function *function;

  /* XXX.  */
  struct symbol_reference *symrefs;

  /* XXX.  */
  int announced_p;

  /* XXX.  */
  int symbols_modified_p;
};

/* XXX.  */

static int
i8symref_init (struct symbol_reference *ref,
	       struct function_note *func, void *unused)
{
  int addr_size = i8note_addr_size (&func->note);
  int slot_offset = (ref - func->symrefs) * addr_size;
  const char *provider, *name;

  ref->slot = func->function->symbols + slot_offset;
  provider = i8func_extract_ident (func->function, ref->slot);
  if (provider == NULL)
    return 0; /* Stop foreach_symbol_reference.  */

  name = i8func_extract_ident (func->function, ref->slot + 2);
  if (name == NULL)
    return 0; /* Stop foreach_symbol_reference.  */

  ref->fullname = i8_make_fullname (provider, name);

  return 1; /* Continue foreach_symbol_reference.  */
}

/* XXX.  */

static int
i8symref_free (struct symbol_reference *ref,
	       struct function_note *func, void *unused)
{
  xfree (ref->fullname);

  return 1; /* Continue foreach_symbol_reference.  */
}

/* XXX.  */

static int
i8symref_resolve (struct symbol_reference *ref,
		  struct function_note *func, void *psdp)
{
  struct infinity_pspace_data *psd
    = (struct infinity_pspace_data *) psdp;
  struct infinity_note *note;
  struct symbol_note *sym;
  CORE_ADDR old_address, new_address;
  int addr_size = i8note_addr_size (&func->note);

  note = i8psd_get_note_by_fullname (psd, ref->fullname);
  if (note == NULL)
    {
      if (debug_infinity)
	debug_printf ("infinity: did not resolve %s\n",
		      ref->fullname);

      return 0; /* Stop foreach_symbol_reference.  */
    }

  sym = i8note_as_symbol (note);
  if (sym == NULL)
    {
      warning (_("GNU Infinity function %s referenced"
		 "non-symbol %s."), func->note.fullname,
	       note->fullname);

      return 0; /* Stop foreach_symbol_reference.  */
    }

  new_address = symbol_note_address (sym);
  old_address = i8_extract_uint (ref->slot, addr_size);

  if (new_address != old_address)
    {
      i8_store_uint (ref->slot, addr_size, new_address);
      func->symbols_modified_p = 1;
    }

  return 1; /* Continue foreach_symbol_reference.  */
}

/* XXX.  */

static int
foreach_symbol_reference (struct function_note *func,
			  int (*worker) (struct symbol_reference *ref,
					 struct function_note *func,
					 void *arg),
			  void *arg)
{
  struct symbol_reference *ref = func->symrefs;
  struct symbol_reference *limit = ref + func->function->num_symbols;

  for (; ref < limit; ref++)
    if (!worker (ref, func, arg))
      return 0;

  return 1;
}

/* Implementation of infinity_note_ops->op_init.  */

static int
function_note_init (struct infinity_note *note)
{
  struct function_note *func = (struct function_note *) note;
  int addr_size = i8note_addr_size (note);
  const gdb_byte *limit;
  int i;

  /* Unpack the function.  */
  func->function = i8func_init (note->elfnote->data,
				note->elfnote->size, addr_size);
  if (func->function == NULL)
    return 0;

  note->fullname = i8_make_fullname (func->function->provider,
				     func->function->name);

  /* Unpack the symbol table.  */
  func->symrefs = xcalloc (func->function->num_symbols,
			   sizeof (struct symbol_reference));
  if (!foreach_symbol_reference (func, i8symref_init, NULL))
    return 0;

  return 1;
}

/* Implementation of infinity_note_ops->op_free.  */

static void
function_note_free (struct infinity_note *note)
{
  struct function_note *func = (struct function_note *) note;

  if (func->function)
    foreach_symbol_reference (func, i8symref_free, NULL);

  xfree (func->symrefs);

  i8func_free (func->function);
}

/* Implementation of infinity_note_ops->op_rebuild.  */

static void
function_note_rebuild (struct infinity_note *note,
		       struct infinity_pspace_data *psd)
{
  struct function_note *func = (struct function_note *) note;
  int all_symbols_resolved;

  func->symbols_modified_p = 0;
  all_symbols_resolved = foreach_symbol_reference (func,
						   i8symref_resolve,
						   psd);

  if (!func->announced_p && all_symbols_resolved)
    {
      if (debug_infinity)
	debug_printf ("infinity: announcing function %s\n",
		      func->note.fullname);

      observer_notify_i8func_load (note->objfile->pspace,
				   func->function);

      func->announced_p = 1;
      return;
    }

  if (func->announced_p && !all_symbols_resolved)
    {
      if (debug_infinity)
	debug_printf ("infinity: withdrawing function %s\n",
		      func->note.fullname);

      observer_notify_i8func_unload (note->objfile->pspace,
				     func->function);

      func->announced_p = 0;
      return;
    }

  if (func->announced_p && func->symbols_modified_p)
    {
      if (debug_infinity)
	debug_printf ("infinity: function %s changed\n",
		      func->note.fullname);

      observer_notify_i8func_change (note->objfile->pspace,
				     func->function);
      return;
    }
}

/* Implementation of infinity_note_ops->op_pre_remove.  */

static void
function_note_pre_remove (struct infinity_note *note)
{
  struct function_note *func = (struct function_note *) note;

  if (func->announced_p)
    {
      if (debug_infinity)
	debug_printf ("infinity: withdrawing function %s\n",
		      func->note.fullname);

      observer_notify_i8func_unload (note->objfile->pspace,
				     func->function);
      func->announced_p = 0;
    }
}

/* Operations for function notes.  */

static const struct infinity_note_ops function_note_ops =
{
  "function",				/* op_typename.  */
  sizeof (struct function_note),	/* op_size.  */
  function_note_init,			/* op_init.  */
  function_note_free,			/* op_free.  */
  NULL,					/* op_relocate.  */
  function_note_rebuild,		/* op_rebuild.  */
  function_note_pre_remove,		/* op_pre_remove.  */
};

/* XXX.  */

static struct infinity_note *
i8psd_get_note_by_elfnote (struct infinity_pspace_data *psd,
			   struct elf_infinity_note *elfnote)
{
  struct infinity_note lookup;
  void **slot;

  lookup.elfnote = elfnote;
  slot = htab_find_slot (psd->by_elfnote, &lookup, NO_INSERT);

  if (slot == NULL)
    return NULL;

  return (struct infinity_note *) *slot;
}

/* XXX.  */

static struct infinity_note *
i8psd_get_note_by_fullname (struct infinity_pspace_data *psd,
			    const char *fullname)
{
  struct infinity_note lookup, *note;
  void **slot;

  lookup.fullname = fullname;
  slot = htab_find_slot (psd->by_fullname, &lookup, NO_INSERT);

  if (slot == NULL)
    return NULL;

  return (struct infinity_note *) *slot;
}

/* XXX.  */

static void
i8psd_remove_note_cleanup (void *notep)
{
  struct infinity_note *note = (struct infinity_note *) notep;
  struct infinity_pspace_data *psd = i8psd_get (note->objfile->pspace);

  htab_remove_elt (psd->by_elfnote, note);
}

/* Add a new note to the per-program-space data.  Called once per
   note whenever a new object file is loaded.  */

static void
i8psd_add_note (struct infinity_pspace_data *psd,
		struct elf_infinity_note *elfnote,
		struct objfile *objfile)
{
  const struct infinity_note_ops *ops;
  struct infinity_note *note;
  struct cleanup *old_chain;
  void **slot;
  int type;

  /* All supported notes start with a two byte note type and a
     two byte version.  infinity_note_ops->op_init expects us
     to have checked the note is at least four bytes long.  */
  if (elfnote->size < 4)
    {
      i8_skip_warning (NOTE_CORRUPT, "size = %s",
		       pulongest (elfnote->size));
      return;
    }

  type = i8_extract_uint (elfnote->data, 2);
  switch (type)
    {
    case INFINITY_NOTE_SYMBOL:
      ops = &symbol_note_ops;
      break;

    case INFINITY_NOTE_FUNCTION:
      ops = &function_note_ops;
      break;

    default:
      i8_skip_warning (NOTE_UNHANDLED, "type = %d", type);
      return;
    }

  note = i8note_alloc (ops);
  note->ops = ops;
  old_chain = make_cleanup (i8note_free, note);

  note->elfnote = elfnote;
  note->objfile = objfile;

  if (!i8note_init (note))
    {
      do_cleanups (old_chain);
      return;
    }

  /* Add the note to the by_elfnote hashtable.  This table frees notes
     on removal, so we need to replace our cleanup with one that will
     remove it from by_elfnote if the add to by_fullname fails.  */
  slot = htab_find_slot (psd->by_elfnote, note, INSERT);
  gdb_assert (*slot == HTAB_EMPTY_ENTRY);
  *slot = note;

  discard_cleanups (old_chain);
  old_chain = make_cleanup (i8psd_remove_note_cleanup, note);

  /* Add the note to the by_fullname hashtable.  Note that fullnames
     are not guaranteed to be unique as they are derived from data
     obtained from the inferior so we handle this by storing multiple
     notes with the same fullname in a list.  */
  slot = htab_find_slot (psd->by_fullname, note, INSERT);
  if (*slot != HTAB_EMPTY_ENTRY)
    note->next = (struct infinity_note *) *slot;
  *slot = note;

  if (debug_infinity)
    debug_printf ("infinity: loaded %s %s%s\n",
		  note->ops->op_typename, note->fullname,
		  note->next == NULL ? "" : " (COLLISION!)");

  discard_cleanups (old_chain);
  psd->needs_rebuild = 1;
}

/* Relocate a note in the per-program-space data.  Called once per
   note whenever an object file is relocated.  */

static void
i8psd_relocate_note (struct infinity_pspace_data *psd,
		     struct elf_infinity_note *elfnote,
		     struct objfile *objfile)
{
  struct infinity_note *note = i8psd_get_note_by_elfnote (psd, elfnote);

  gdb_assert (note != NULL);

  i8note_relocate (note, psd);
}

/* Remove a note from the per-program-space data.  Called once per
   note whenever an object file is unloaded.  */

static void
i8psd_remove_note (struct infinity_pspace_data *psd,
		   struct elf_infinity_note *elfnote,
		   struct objfile *objfile)
{
  struct infinity_note *note = i8psd_get_note_by_elfnote (psd, elfnote);
  void **slot;

  gdb_assert (note != NULL);

  i8note_prepare_for_removal (note);

  /* Remove the note from the by_fullname hashtable.  */
  slot = htab_find_slot (psd->by_fullname, note, NO_INSERT);
  gdb_assert (slot != NULL);
  if (*slot == note && note->next == NULL)
    htab_remove_elt (psd->by_fullname, note);
  else
    {
      /* There is more than one note with this fullname.  */
      struct infinity_note **last = (struct infinity_note **) slot;
      struct infinity_note *cur;

      for (cur = *last; cur != NULL; cur = cur->next)
	{
	  if (cur == note)
	    {
	      *last = cur->next;
	      break;
	    }
	}
    }

  /* Removal from the by_elfnote hashtable frees the note.  */
  htab_remove_elt (psd->by_elfnote, note);

  psd->needs_rebuild = 1;
}

/* Per-program-space data key.  */
static const struct program_space_data *infinity_pspace_data;

/* Get the per-program-space data.  If none is found, allocate and
   initialize one.  This function always returns a valid object.  */

static struct infinity_pspace_data *
i8psd_get (struct program_space *pspace)
{
  struct infinity_pspace_data *psd;

  psd = program_space_data (pspace, infinity_pspace_data);
  if (psd != NULL)
    return psd;

  psd = XCNEW (struct infinity_pspace_data);
  set_program_space_data (pspace, infinity_pspace_data, psd);

  psd->by_elfnote = htab_create_alloc (1,
				       i8note_hash_elfnote,
				       i8note_equal_elfnote,
				       i8note_free,
				       xcalloc, xfree);

  psd->by_fullname = htab_create_alloc (1,
					i8note_hash_fullname,
					i8note_equal_fullname,
					NULL,
					xcalloc, xfree);

  return psd;
}

/* Free the per-program-space data.  */

static void
i8psd_cleanup (struct program_space *pspace, void *arg)
{
  struct infinity_pspace_data *psd = arg;

  htab_delete (psd->by_fullname);
  htab_delete (psd->by_elfnote);
  xfree (psd);
}

/* XXX.  */

static int
i8psd_rebuild_note (void **slot, void *psdp)
{
  struct infinity_note *note = (struct infinity_note *) *slot;

  i8note_rebuild (note, (struct infinity_pspace_data *) psdp);

  return 1; /* Continue scan.  */
}

/* Call WORKER for each infinity note in OBJFILE, then XXX...  */

static void
i8psd_update (struct objfile *objfile,
	      void (*worker) (struct infinity_pspace_data *,
			      struct elf_infinity_note *,
			      struct objfile *))
{
  struct infinity_pspace_data *psd = NULL;
  struct elf_infinity_note *note;

  if (objfile == NULL
      || objfile->obfd == NULL
      || bfd_get_flavour (objfile->obfd) != bfd_target_elf_flavour)
    return;

  /* Invoke the worker for each note in the file.  */
  for (note = elf_tdata (objfile->obfd)->infinity_note_head;
       note != NULL; note = note->next)
    {
      if (psd == NULL)
	psd = i8psd_get (objfile->pspace);

      worker (psd, note, objfile);
    }

  /* Rebuild the functions if necessary.  */
  if (psd != NULL && psd->needs_rebuild)
    {
      htab_traverse (psd->by_elfnote, i8psd_rebuild_note, psd);

      psd->needs_rebuild = 0;
    }
}

/* Observer, called whenever a new object file is loaded.  */

static void
infinity_new_objfile (struct objfile *objfile)
{
  i8psd_update (objfile, i8psd_add_note);
}

/* Observer, called whenever an object file is relocated.  */

static void
infinity_relocate_objfile (struct objfile *objfile)
{
  i8psd_update (objfile, i8psd_relocate_note);
}

/* Observer, called whenever an object file is unloaded.  */

static void
infinity_free_objfile (struct objfile *objfile)
{
  i8psd_update (objfile, i8psd_remove_note);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_infinity;

void
_initialize_infinity (void)
{
  /* Register our debug flag.  */
  add_setshow_boolean_cmd ("infinity", class_maintenance,
			   &debug_infinity, _("\
Set debugging of GNU Infinity functions."), _("\
Show debugging of GNU Infinity functions."), _("\
Enables printf debugging output."),
			   NULL,
			   NULL,
			   &setdebuglist, &showdebuglist);

  /* Register our per-program-space data.  */
  infinity_pspace_data
    = register_program_space_data_with_cleanup (NULL, i8psd_cleanup);

  /* Notice when object files are loaded, unloaded and relocated.  */
  observer_attach_new_objfile (infinity_new_objfile);
  observer_attach_relocate_objfile (infinity_relocate_objfile);
  observer_attach_free_objfile (infinity_free_objfile);
}
