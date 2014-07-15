import os
import sys
import subprocess

TESTFILE = os.path.splitext(os.path.realpath(sys.argv[0]))[0] + ".tests"
[gdbexe] = sys.argv[1:]
gdbexe = os.path.realpath(gdbexe)
datadir = os.path.join(os.path.dirname(gdbexe), "data-directory")

passes = fails = 0
for line in open(TESTFILE).xreadlines():
    symbol = line.split("#", 1)[0].rstrip()
    if not symbol:
        continue
    result = subprocess.call((gdbexe, "-nx", "-batch",
                              "--data-directory", datadir,
                              "-ex", "set lang c++",
                              "-ex", "maint demangle %s" % symbol))
    if result == -11:
        print "Segmentation fault"
    success = {0: True, 1: False, -11: False}[result]
    print {True: "\x1B[32mpass\x1b[0m",
           False: "\x1B[31mFAIL\x1b[0m"}[success]
    if success:
        passes += 1
    else:
        fails += 1
print "%d passes, %d fails" % (passes, fails)
