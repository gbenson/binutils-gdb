#include <stdio.h>
#include <stdlib.h>
#include <demangle.h>

const char *mangled = "_ZSt7forwardIRN1x14refobjiteratorINS0_3refINS0_" \
  "4mime30multipart_section_processorObjIZ15get_body_parserIZZN14mime" \
  "_processor21make_section_iteratorERKNS2_INS3_10sectionObjENS0_10pt" \
  "rrefBaseEEEbENKUlvE_clEvEUlSB_bE_ZZNS6_21make_section_iteratorESB_" \
  "bENKSC_clEvEUlSB_E0_ENS1_INS2_INS0_20outputrefiteratorObjIiEES8_EE" \
  "EERKSsSB_OT_OT0_EUlmE_NS3_32make_multipart_default_discarderISP_EE" \
  "EES8_EEEEEOT_RNSt16remove_referenceISW_E4typeE";

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
