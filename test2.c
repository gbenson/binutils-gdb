void
dummy(void)
{
__asm__ __volatile__ (
  ".pushsection .note.infinity, \"a\", \"note\"\n"
  ".byte 4, 0, 0, 0\n"  /* Name size */
  ".byte 17, 0, 0, 0\n" /* Desc size INCLUDING 2 BYTE VERSION */
  ".byte 23, 0, 0, 0\n" /* Note type */
  ".string \"GNU\"\n"
  ".align 4\n"
  ".byte 1\n"  /* Major version */
  ".byte 0\n"  /* Minor version */
  ".string \"Hello again...\"\n"
  ".align 4\n"
  ".popsection\n");
}
