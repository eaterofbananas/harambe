// SAFE
#include <stdlib.h>
#include <stdio.h>

int main() {
  int* p = (int*)malloc(sizeof(int));
  *p = 0x41424344;
  short* q = (short*)((char*)p + 2);
  *q = 0;
  return 0;
}
