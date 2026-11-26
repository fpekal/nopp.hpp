// Compile this file first using
// g++ -std=c++26 -o nopp nopp.cpp
//
// Then use ./nopp

#define NOPP_IMPLEMENTATION
#include "../../nopp.hpp"

#include <cassert>
#include <format>

int main(int argc, char *argv[]) {
  NOPP::NOPP nopp{argc, argv};
  nopp.rebuild_urself();

  // NOPP has support for boolean, int and string options.

  // Boolean flags support -- and - prefixes.
  // When user writes -abcd NOPP interprets it as 4 separate flags
  // -a, -b, -c and -d
  //
  // `option<>()` returns `std::optional`. Optional is empty when user didn't
  // provide this option when running `./nopp`.
  // Boolean options ALWAYS have value. When the user didn't provide some
  // option, the value is just `false`.
  bool native = nopp.option<bool>("--native").value();
  bool n = nopp.option<bool>("-n").value();

  // For deault values we can use `std::optional`'s `value_or()`, or
  // `has_value()`.
  int opt_level = nopp.option<int>("--opt_level").value_or(1);
  // We can use these variables as we wish. It's just a normal C++, remember?
  assert(opt_level >= 0 && opt_level <= 3);

  std::string opt_str = std::format("-O{}", opt_level);

  nopp.get_default()->depend_on(NOPP::Step::create(
      {"echo", "g++", "-o", "main", (native || n) ? "-march=native" : "",
       opt_str, "main.cpp"}));

  nopp.build();
}
