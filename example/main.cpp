#include <iostream>

#include "fib.h"

int main(int argc, char *argv[]) {
  int n = 10;

  if (argc >= 2) {
    if (argv[1] != std::string{""})
      n = std::stoi(argv[1]);
  }

  std::cout << "fib(" << n << ") = " << fib(n) << '\n';
}
