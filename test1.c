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
  "     .4byte 102f-101f\n"  /* namesz */
  "     .4byte 104f-103f\n"  /* descsz */
  "     .4byte 23\n"         /* NT_GNU_INFINITY */
  "101: .string \"GNU\"\n"
  "102: .balign 4\n"
  "103: .byte 1\n"               /* version */
  "     .byte 0\n"               /* reserved MUST BE ZERO */
  "     .byte 0\n"               /* reserved MUST BE ZERO */
  "     .byte 0\n"               /* reserved MUST BE ZERO */
  "     .string \"td_ta_new\"\n" /* name + '\0' */
  "104: .balign 4\n"
  "     .popsection\n");
}
