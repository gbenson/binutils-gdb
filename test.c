/* gcc -shared -o libtest.so test.c */
/* eu-readelf -n libtest.so */

#include <sys/sdt.h>

static void
dummyfunc ()
{
  __asm__ __volatile__ (
    _SDT_ASM_3 (.pushsection .note.infinity, "a", "note")
    _SDT_ASM_1 (.byte 72)
    _SDT_ASM_1 (.byte 101)
    _SDT_ASM_1 (.byte 108)
    _SDT_ASM_1 (.byte 108)
    _SDT_ASM_1 (.byte 111)
    _SDT_ASM_1 (.byte 32)
    _SDT_ASM_1 (.byte 119)
    _SDT_ASM_1 (.byte 111)
    _SDT_ASM_1 (.byte 114)
    _SDT_ASM_1 (.byte 108)
    _SDT_ASM_1 (.byte 100)
    _SDT_ASM_1 (.popsection));
}

int main()
{
}
