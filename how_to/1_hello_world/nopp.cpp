// Compile this file first using
// g++ -std=c++26 -o nopp nopp.cpp
//
// Then use ./nopp

#define NOPP_IMPLEMENTATION
#include "../../nopp.hpp"

int main(int argc, char *argv[]) {
  NOPP::NOPP nopp{argc, argv};
  // Automatically rebuild ./nopp when something changes in nopp.cpp (this file)
  nopp.rebuild_urself();
  // You can force-rebuild nopp by using `./nopp rebuild_nopp`

  // Create a *step* of the building process
  // It won't run by itself
  auto hello = NOPP::Step::create({"echo", "Hello"});
  auto world = NOPP::Step::create({"echo", " world"});

  // Steps can have dependencies.
  // When `world` step will try to run, it will run all their dependencies first
  // and wait for them to complete.
  world->depend_on(hello);
  // So `hello` will run first, then `world`.

  // `nopp` defines "actions".
  // Plain `./nopp`, runs the *default* action.
  //
  // `./nopp foobar`, will run the "foobar" action.

  // `get_default()` returns default action
  // Actions are steps under the hood. So you can attach your steps to them by
  // just adding them as a dependency.
  nopp.get_default()->depend_on(world);

  // Define the "foobar" action and attach `hello` step to it.
  nopp.action("foobar")->depend_on(hello);

  // Run the build process!
  nopp.build();
}
