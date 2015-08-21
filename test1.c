/* 1. Name is NUL-terminated.  The NUL is included in namesz.

   2. Name is padded, if necessary, to ensure 4-byte alignment
      for the descriptor.  Such padding is not included in namesz.

   3. The descriptor is padded, if necessary, to ensure 4-byte
      alignment for the next note entry.  Such padding is not
      included in descsz. */

int
main (int argc, char *argv[])
{
__asm__ __volatile__ (
  "	.pushsection .note.infinity, \"a\", \"note\"\n"
  "     .balign 4\n" /* should be unnecessary, but... */
  "     .4byte 102f-101f\n"  /* namesz */
  "     .4byte 104f-103f\n"  /* descsz */
  "     .4byte 23\n"         /* NT_GNU_INFINITY */
  "101: .string \"GNU\"\n"
  "102: .balign 4\n"
  "103: .2byte 1\n"                   /* version */
  "     .2byte 0\n"                   /* reserved1 (must be zero) */
  "     .2byte 78\n"                  /* num_constants */
  "     .2byte 56\n"                  /* num_args */
  "     .2byte 910\n"                 /* max_locals (must be >= num_args) */
  "     .2byte 1112\n"                /* max_stack */
  "     .4byte 0\n"                   /* reserved2 (must be zero) */
  "     .string \"libpthread\"\n"     /* provider + '\0' */
  "     .string \"ta_new\"\n"         /* name + '\0' */
  "     .byte 23, 4, 17, 2\n"         /* instructions */
  "104: .balign 4\n"
  "     .popsection\n");
}
