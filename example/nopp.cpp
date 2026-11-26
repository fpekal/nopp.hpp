#define NOPP_IMPLEMENTATION
#include "../nopp.hpp"

int main(int argc, char *argv[]) {
  NOPP::NOPP nopp{argc, argv};
  nopp.rebuild_urself();

  // Enable memoization in fib?
  auto memo = nopp.option<bool>("--memo").value();

  auto main_o = NOPP::Step::create({"g++", "-c", "-o", "main.o", "main.cpp"});

  std::vector<std::string> fib_comp;
  fib_comp.push_back("g++");
  fib_comp.push_back("-c");
  if (memo) {
    fib_comp.push_back("-DMEMO");
  }
  fib_comp.push_back("-o");
  fib_comp.push_back("fib.o");
  fib_comp.push_back("fib.cpp");
  auto fib_o = NOPP::Step::create(fib_comp);

  auto linking = NOPP::Step::create({"g++", "-o", "main", "main.o", "fib.o"})
                     ->depend_on(main_o)
                     ->depend_on(fib_o);

  nopp.get_default()->depend_on(linking);

  std::string arg = nopp.option<std::string>("--arg").value_or("");

  auto run = NOPP::Step::create({"./main", arg})->depend_on(linking);
  nopp.action("run")->depend_on(run);

  nopp.build();
}
