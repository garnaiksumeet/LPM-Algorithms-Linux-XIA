/*
   xidsort.h
*/

void xidsort(list *base, size_t narray, size_t size, int (*compare)(const void *, const void *));
void xidentrysort(entry_t entry, size_t narray, size_t size, int (*compare)(const void *, const void *));
