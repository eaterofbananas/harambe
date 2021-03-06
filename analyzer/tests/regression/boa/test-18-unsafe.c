// DEFINITE UNSAFE
#include <stdio.h>
#include <stdlib.h>

// We just use a pointer as an array
// Same as test-8-unsafe.c but using calloc
#define MAX_ARRAY 10
// To test loops that decrements a counter
int main(int argc, char** argv) {
  int n = MAX_ARRAY - 1;
  int* p = (int*)calloc(n, sizeof(int));
  int i;
  for (i = n - 1; i >= 0; i--) {
    p[i] = i;
  }
  // for underflow check
  // printf("%d\n", p[i+1]);
  // for overflow check
  printf("%d\n", p[MAX_ARRAY - 1]);
  return 0;
}
