import os
import subprocess

sym_a = "_QueueNotification_QueueController__X4PPM_A_INotice___Z"
sym_a = "_Z%d%sv" % (len(sym_a), sym_a)
sym_b = "_QueueNotification_QueueController__$4PPPPPPPM_A_INotice___Z"
assert len(sym_a) == len(sym_b)

binfile = "test"
srcfile = binfile + ".cc"

open(srcfile, "w").write("""\
#include <stdio.h>

void
hello (void)
{
  puts ("hello");
}

void
%s (void)
{
  hello ();
}

int
main (int argc, char *argv[])
{
  %s ();
  return 0;
}
""" % (sym_a, sym_a))

subprocess.check_call(("/usr/bin/g++", "-g", "-O0", srcfile, "-o", binfile))
os.unlink(srcfile)

bin_a = open(binfile).read()
bin_b = bin_a.replace(sym_a, sym_b)
assert bin_b != bin_a
open(binfile, "w").write(bin_b)
os.chmod(binfile, 0755)
