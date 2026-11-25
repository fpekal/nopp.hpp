#define NOPP_IMPLEMENTATION
#include "nopp.hpp"

int main(int argc, char *argv[]) {
  auto base = NOPP::Step::create({"echo", "siema"});

  auto b1 = NOPP::Step::create({"sleep", "5"})->depend_on(base);
  auto b2 = NOPP::Step::create({"echo", "elo", "320"})->depend_on(base);

  auto s = NOPP::Step::create()->depend_on(b1)->depend_on(b2);
  s->run();
}
