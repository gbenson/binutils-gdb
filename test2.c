void
dummy(void)
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
  "     .string \"td_ta_map_lwp2thr\"\n" /* name + '\0' */
  "104: .balign 4\n"
  "     .popsection\n");
}
