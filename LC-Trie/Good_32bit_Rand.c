/*
   Pierre L'Ecuyer, Efficient and Portable Combined Random Number Generators,
   CACM, June 1988, Volume 31, Number 6, p. 742-749 & 774.

   The following implementation works as long as the machine can represent all
   integers in the range [-2^31+85, 2^31-85].
*/

static long seed1 = 1;  /* global seeds */
static long seed2 = 1;

long good_lrand(void)
{
  long Z, k;

  k = seed1/53668;

  seed1 = 40014 * (seed1 - k * 53668) - k * 12211;
  if (seed1 < 0)
    seed1 = seed1 + 2147483563;

  k = seed2/52774;
  seed2 = 40692 * (seed2 - k * 52774) - k * 3791;
  if (seed2 < 0)
    seed2 = seed2 + 2147483399;

  Z = seed1 - seed2;
  if (Z < 1)
    Z = Z + 2147483562;

  return Z;
}

double good_drand(void)
{
   return 4.656613e-10 * good_lrand();
}

void good_srand(long init1, long init2)
{
   seed1 = init1;
   seed2 = init2;
}
