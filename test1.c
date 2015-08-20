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
  ".balign 4\n"  /* Just in case */
  ".4byte 4\n"  /* Name size */
  ".4byte 15\n" /* Desc size INCLUDING 2 BYTE VERSION */
  ".4byte 23\n" /* Note type */
  ".string \"GNU\"\n"
  ".balign 4\n"
  ".byte 1\n"  /* Major version */
  ".byte 0\n"  /* Minor version */
  ".string \"Hello world!\"\n"
  ".balign 4\n"
  ".popsection\n");
}
