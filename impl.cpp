#include "nopp.hpp"
#include <algorithm>

bool NOPP::has_space(std::string str) {
  return std::ranges::fold_left(
      str, false, [](bool prev, char c) { return prev || isspace(c); });
}
