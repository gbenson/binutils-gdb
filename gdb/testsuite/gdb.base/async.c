

#ifdef PROTOTYPES
int
foo (void)
#else
int
foo ()
#endif
{
 int y;
 volatile int x;

 x = 5; x = 5;
 y = 3;

 return x + y;
}

#ifdef PROTOTYPES
int
main (void)
#else
int
main ()
#endif
{
 int y, z;
 
 y = 2;
 z = 9;
 y = foo ();
 z = y;
 y = y + 2; /* jump here */
 y = baz ();
 return 0; /* until here */
}


#ifdef PROTOTYPES
int
baz (void)
#else
int
baz ()
#endif
{ 
  return 5;
}
