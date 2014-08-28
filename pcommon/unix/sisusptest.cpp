#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void sigusr_handler(int sig)
{
   const char msg[] = "SIGUSR1 handler\n" ;
   write(1, "SIGUSR1 handler\n", sizeof msg - 1) ;
}

const int SIGRESUMETHREAD = (SIGRTMIN + SIGRTMAX)/2 ;

int main()
{
   printf("pid=%d\n", getpid()) ;
   struct sigaction sigusr1 ;
   sigusr1.sa_flags = 0 ;
   sigemptyset(&sigusr1.sa_mask) ;
   sigusr1.sa_handler = sigusr_handler ;

   // Block everything
   sigset_t every_signal ;
   sigset_t resume_signal ;
   sigset_t current_set ;

   sigfillset(&every_signal) ;

   sigemptyset(&resume_signal) ;
   sigaddset(&resume_signal, SIGRESUMETHREAD) ;

   sigaction(SIGUSR1, &sigusr1, NULL) ;
   sigprocmask(SIG_BLOCK, &resume_signal, NULL) ;
   pthread_sigmask(SIG_BLOCK, &every_signal, &current_set) ;
   printf("Waiting for %d...\n", SIGRESUMETHREAD) ;
   int received = 0 ;
   sigwait(&resume_signal, &received) ;
   printf("Got %d!\n", received) ;
   pthread_sigmask(SIG_SETMASK, &current_set, NULL) ;
   sleep(60) ;
}
