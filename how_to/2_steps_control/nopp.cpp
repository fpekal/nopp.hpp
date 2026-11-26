// Compile this file first using
// g++ -std=c++26 -o nopp nopp.cpp
//
// Then use ./nopp

#define NOPP_IMPLEMENTATION
#include "../../nopp.hpp"

int main(int argc, char *argv[]) {
  NOPP::NOPP nopp{argc, argv};
  nopp.rebuild_urself();

  // We can chain many steps using `add()` notation.
  // `add()` creates a new step, adds previous step as a dependency and returns
  // this new step.
  //
  // So in other words, these steps will run from first to last.
  auto long_step = NOPP::Step::create({"sleep", "5"})
                       ->add({"echo", "halfway there"})
                       ->add({"sleep", "5"})
                       ->add({"echo", "done!"});
  nopp.action("first")->depend_on(long_step);

  auto another_long_step = NOPP::Step::create({"echo", "starting..."})
                               ->add({"sleep", "13"})
                               ->add({"echo", "also done!"});
  nopp.action("second")->depend_on(another_long_step);

  // We can `create()` an EMPTY step by just omitting the command.
  // It's useful to organize steps.
  //
  // In this example, this empty step depends on two previous steps
  auto merge_step = NOPP::Step::create()
                        ->depend_on(long_step)
                        ->depend_on(another_long_step)
                        ->add({"echo", "joined!"});

  nopp.get_default()->depend_on(merge_step);

  nopp.build();
}
