/* gcc -shared -o libtest.so test.c */
/* eu-readelf -n libtest.so */

/* NB objcopy --add-section can add to pre-built files.*/
/* NB objcopy is unconditionally usable at glibc build time */

#include <sys/sdt.h>

#define NT_GNU_INFINITY 5

/* NB Add the above to glibc/src/elf/elf.h */

static void
dummyfunc ()
{
  /* .pushsection, .incbin, .popsection */
  __asm__ __volatile__ (
    _SDT_ASM_3 (.pushsection .note.infinity, "a", "note")
    _SDT_ASM_1 (.4byte 4)  /* Name size */
    _SDT_ASM_1 (.4byte 13) /* Desc size */
    _SDT_ASM_1 (.4byte NT_GNU_INFINITY)
    _SDT_ASM_1 (.string "GNU")
    _SDT_ASM_1 (.align 4)
    _SDT_ASM_1 (.string "Hello world!")
    _SDT_ASM_1 (.align 4)
    _SDT_ASM_1 (.popsection));
}

int main()
{
}
