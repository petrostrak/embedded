#define _POSIX_C_SOURCE 200809L /* for alarm(); -std=c11 hides POSIX decls */

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "flag.h"

static void on_alarm(int sig)
{
  (void)sig;
  set_ready(); /* lives in flag.c: invisible to main.c's optimiser */
}

int main(void)
{
  signal(SIGALRM, on_alarm);
  alarm(1); /* something outside main() will flip the flag */

  puts("polling...");
  fflush(stdout);

  while (!ready)
  { /* no side effects, no calls, nothing volatile */
    /* spin */
  }

  puts("saw ready == 1");
  return 0;
}
