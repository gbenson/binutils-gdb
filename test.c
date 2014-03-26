#include <stdio.h>
#include <stdlib.h>
#include <demangle.h>

const char *mangled = "_ZNSt9_Any_data9_M_accessIPZN13ThreadManager10fu" \
  "tureTaskISt5_BindIFSt7_Mem_fnIM6RunnerFvvEEPS5_EEEEvOT_EUlvE_EERSC_v";

int
main (int argc, char *argv[])
{
  const char *demangled;
  FILE *fp;

  fp = fopen ("test.in", "w");
  if (fp != NULL)
    {
      fputs (mangled, fp);
      fclose (fp);
    }

  demangled = cplus_demangle (mangled, DMGL_AUTO | DMGL_ANSI | DMGL_PARAMS);

  if (demangled == NULL)
    {
      puts ("Demangler failed");
      exit (EXIT_FAILURE);
    }
  puts (demangled);

  fp = fopen ("test.out", "w");
  if (fp != NULL)
    {
      fputs (demangled, fp);
      fclose (fp);
    }

  exit (EXIT_SUCCESS);
}
