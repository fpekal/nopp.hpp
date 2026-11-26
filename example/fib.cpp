#include "fib.h"

#ifndef MEMO
int fib(int n) {
  if (n <= 2)
    return n;

  return fib(n - 1) + fib(n - 2);
}
#else
#include <map>

int fib(int n) {
  static std::map<int, int> memo;
  auto found = memo.find(n);
  if (found != memo.end())
    return found->second;

  if (n <= 2) {
    memo.insert({n, n});
    return n;
  }

  int result = fib(n - 1) + fib(n - 2);
  memo.insert({n, result});
  return result;
}
#endif
