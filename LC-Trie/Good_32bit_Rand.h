/*
   Good_32bit_Rand.h

   Pierre L'Ecuyer, Efficient and Portable Combined Random Number Generators,
   CACM, June 1988, Volume 31, Number 6, p. 742-749 & 774.

   The following implementation works as long as the machine can represent all
   integers in the range [-2^31+85, 2^31-85].
*/

long good_lrand(void);
double good_drand(void);
void good_srand(long seed1, long seed2);

