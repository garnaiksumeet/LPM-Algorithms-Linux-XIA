/*
   qsort.h

   J. L. Bentley and M. D. McIlroy. Engineering a sort function.
   Software---Practice and Experience, 23(11):1249-1265.
*/

void quicksort(char *a, size_t n, size_t es, int (*cmp) ());
