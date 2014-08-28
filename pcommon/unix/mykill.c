#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main(int argc, char *argv[])
{
   int pid, sig ;
   if (argc != 3)
   {
      fprintf(stderr, "mykill sig pid\n") ;
      return 1 ;
   }
   pid = atoi(argv[2]) ;
   if (pid < 2)
   {
      fprintf(stderr, "Bad pid\n") ;
      return 1 ;
   }
   sig = atoi(argv[1]) ;
   if (sig < 0 || sig > 1023)
   {
      fprintf(stderr, "Bad signal\n") ;
      return 1 ;
   }
   printf("Killing process %d with signal %d\n", pid, sig) ;
   if (kill(pid, sig))
   {
      perror(NULL) ;
      return 3 ;
   }
   return 0 ;
}
