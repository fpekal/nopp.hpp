#define NOPP_IMPLEMENTATION
#include "nopp.hpp"

int main(int argc, char *argv[]) {
  NOPP::NOPP nopp{argc, argv};
  nopp.rebuild_urself();

  auto hello = NOPP::Step::create({"echo", "Hello world!"});
  nopp.get_default()->depend_on(hello);

  nopp.build();
}
