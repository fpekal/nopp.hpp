#include "nopp.hpp"
#include <algorithm>

bool NOPP::has_space(std::string str) {
  return std::ranges::fold_left(
      str, false, [](bool prev, char c) { return prev || isspace(c); });
}

namespace NOPP {
template <> std::optional<bool> NOPP::option<bool>(std::string_view str) {
  for (auto &opt : options) {
    if (opt.first == str)
      return true;
  }

  return false;
}

template <> std::optional<int> NOPP::option<int>(std::string_view str) {
  for (auto &opt : options) {
    if (opt.first == str)
      return std::stoi(opt.second);
  }

  return {};
}

template <>
std::optional<std::string> NOPP::option<std::string>(std::string_view str) {
  for (auto &opt : options) {
    if (opt.first == str)
      return opt.second;
  }

  return {};
}
} // namespace NOPP
