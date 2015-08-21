void
dummy(void)
{
__asm__ __volatile__ (
  "	.pushsection .note.infinity, \"a\", \"note\"\n"
  "     .balign 4\n" /* should be unnecessary, but... */
  "     .4byte 102f-101f\n"  /* namesz */
  "     .4byte 104f-103f\n"  /* descsz */
  "     .4byte 23\n"         /* NT_GNU_INFINITY */
  "101: .string \"GNU\"\n"
  "102: .balign 4\n"
  "103: .byte 0, 1\n"                   /* version */
  "     .byte 0, 0\n"                   /* reserved1 (must be zero) */
  "     .byte 102, 103\n"               /* num_constants */
  "     .byte 100, 101\n"               /* num_args */
  "     .byte 104, 105\n"               /* max_locals (must be >= num_args) */
  "     .byte 106, 107\n"               /* max_stack */
  "     .byte 0, 0, 0, 0\n"             /* reserved2 (must be zero) */
  "     .string \"libpthread\"\n"       /* provider + '\0' */
  "     .string \"ta_map_lwp2thr\"\n"   /* name + '\0' */
  "     .byte 14, 23, 12, 118\n"        /* instructions */
  "     .byte 12, 242, 42\n"
  "104: .balign 4\n"
  "     .popsection\n");
}
