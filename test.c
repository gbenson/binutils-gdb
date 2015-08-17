/* gcc -shared -o libtest.so test.c */
/* eu-readelf -n libtest.so */
/* OR:
   gcc test.c -o libpthread-test
   (GDB will check_for_thread_db if objfile name has "/libpthread" in it) */

/* NB objcopy --add-section can add to pre-built files.*/
/* NB objcopy is unconditionally usable at glibc build time */

/* NB a program called rpcgen is built at glibc build time
   which is then run to emit stuff the compiler then compiles

   -or-

   <codonell> Python is currently a dependency of our microbenchmarks
              framework, and I expect it could easily become a build
              dependency, since we already depend on Perl.
   <gbenson> oh, that's great, I'll do it in python and say you need
             python installed to have the new interface built
   <gbenson> if not you can use libthread_db
   <codonell> That would be acceptable IMO, and I would support that.  */

#include <sys/sdt.h>

#define NT_GNU_INFINITY 23

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
    _SDT_ASM_1 (.byte 1)  /* Major version */
    _SDT_ASM_1 (.byte 0)  /* Minor version */
    _SDT_ASM_1 (.string "Hello world!")
    _SDT_ASM_1 (.align 4)
    _SDT_ASM_1 (.popsection));
}

int main()
{
}

static const char infinity_version[] __attribute_used__ = "23.4.209";
