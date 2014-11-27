/* Header for GDB line completion.
   Copyright (C) 2000-2014 Free Software Foundation, Inc.

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

#if !defined (COMPLETER_H)
#define COMPLETER_H 1

#include "gdb_vecs.h"
#include "command.h"

extern VEC (char_ptr) *complete_line (const char *text,
				      const char *line_buffer,
				      int point);

extern char *readline_line_completion_function (const char *text,
						int matches);

extern VEC (char_ptr) *noop_completer (struct cmd_list_element *,
				       const char *, const char *);

extern VEC (char_ptr) *filename_completer (struct cmd_list_element *,
					   const char *, const char *);

extern VEC (char_ptr) *expression_completer (struct cmd_list_element *,
					     const char *, const char *);

extern VEC (char_ptr) *location_completer (struct cmd_list_element *,
					   const char *, const char *);

extern VEC (char_ptr) *command_completer (struct cmd_list_element *,
					  const char *, const char *);

extern VEC (char_ptr) *signal_completer (struct cmd_list_element *,
					 const char *, const char *);

extern char *get_gdb_completer_quote_characters (void);

extern char *gdb_completion_word_break_characters (void);

/* Set the word break characters array to the corresponding set of
   chars, based on FN.  This function is useful for cases when the
   completer doesn't know the type of the completion until some
   calculation is done (e.g., for Python functions).  */

extern void set_gdb_completion_word_break_characters (completer_ftype *fn);

/* Exported to linespec.c */

extern const char *skip_quoted_chars (const char *, const char *,
				      const char *);

extern const char *skip_quoted (const char *);

/* Object to track how many unique completions have been generated.
   Used to limit the size of generated completion lists.  */

typedef htab_t completion_tracker_t;

/* Create a new completion tracker.  */

extern completion_tracker_t new_completion_tracker (void);

/* Make a cleanup to free a completion tracker.  */

extern struct cleanup *make_cleanup_free_completion_tracker
		      (completion_tracker_t tracker);

/* Add the completion NAME to the list of generated completions if
   it is not there already.  Throw TOO_MANY_COMPLETIONS_ERROR if
   max_completions >= 0 and the number of generated completions is
   greater than max_completions.  Do nothing if max_completions is
   negative.  */

extern void maybe_limit_completions (completion_tracker_t tracker,
				     char *name);

#endif /* defined (COMPLETER_H) */
