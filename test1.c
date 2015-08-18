// Name is null-terminated

/* Padding is present, if necessary, to ensure 4-byte alignment for
 the descriptor.  Such padding is not included in namesz. */

/* Padding is present, if necessary, to ensure 4-byte alignment for
the next note entry.  Such padding is not included in descsz. */

int
main (int argc, char *argv[])
{
__asm__ __volatile__ (
  ".pushsection .note.infinity, \"a\", \"note\"\n"
  ".byte 4, 0, 0, 0\n"  /* Name size */
  ".byte 15, 0, 0, 0\n" /* Desc size INCLUDING 2 BYTE VERSION */
  ".byte 23, 0, 0, 0\n" /* Note type */
  ".string \"GNU\"\n"
  ".align 4\n"
  ".byte 1\n"  /* Major version */
  ".byte 0\n"  /* Minor version */
  ".string \"Hello world!\"\n"
  ".align 4\n"
  ".popsection\n");
}
