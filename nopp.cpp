#define NOPP_IMPLEMENTATION
#include "nopp.hpp"

#include <format>

int main(int argc, char *argv[]) {
  NOPP::NOPP nopp{argc, argv};
  nopp.rebuild_urself();

  std::optional<bool> pro_features = nopp.option<bool>("--pro");
  std::optional<int> cores = nopp.option<int>("--cores");

  std::string core_str = std::format("-j{}", cores.value_or(1));

  auto s = NOPP::Step::create(
      {"echo", "make", core_str, pro_features.value() ? "-Dpro" : ""});

  nopp.get_default()->depend_on(s);
  nopp.build();
}
