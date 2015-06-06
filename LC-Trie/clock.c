/*
   clock.c

   Simple cpu-time measurement

   The code presented in this file has been tested with care but is
   not guaranteed for any purpose. The writer does not offer any
   warranties nor does he accept any liabilities with respect to
   the code.

   Stefan Nilsson, 2 jan 1997.

   Laboratory of Information Processing Science
   Helsinki University of Technology
   Stefan.Nilsson@hut.fi
*/

#include <time.h>

static clock_t startclock, stopclock;

void clockon() { startclock = clock(); }

void clockoff() { stopclock = clock(); }

double gettime(void)
{
   return (stopclock-startclock) / (double) CLOCKS_PER_SEC;
}
