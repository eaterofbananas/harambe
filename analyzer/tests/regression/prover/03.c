extern void __ikos_assert(int);
extern int __ikos_unknown();

/*
 * "nested4.c" from InvGen benchmark suite
 */

int main() {
  int i, k, n, l;
  n = __ikos_unknown();
  l = __ikos_unknown();
  // assume(l>0);
  if (l > 0) {
    for (k = 1; k < n; k++) {
      for (i = l; i < n; i++) {
      }
      for (i = l; i < n; i++) {
        __ikos_assert(1 <= i);
      }
    }
  }
}
