#include <stdio.h>
#include <stdlib.h>
#include <demangle.h>

#define MAXLEN 253
#define ALPMIN 33
#define ALPMAX 127

int
main (int argc, char *argv[])
{
  char symbol[2 + MAXLEN + 1] = "_Z";

  srand (time (NULL));
  while (1)
    {
      char *buffer = symbol + 2;
      int length, i;

      length = rand () % MAXLEN;
      for (i = 0; i < length; i++)
	*(buffer++) = (rand () % (ALPMAX - ALPMIN)) + ALPMIN;

      *(buffer++) = '\0';

      cplus_demangle (symbol, DMGL_AUTO | DMGL_ANSI | DMGL_PARAMS);
    }
}
