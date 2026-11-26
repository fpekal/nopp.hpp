#pragma once
#include <filesystem>
#include <future>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <string_view>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace NOPP {

bool has_space(std::string str);

// Step is a separate process
// It can depend on another steps
class Step {
public:
  enum Status {
    READY,
    RUNNING,
    DONE,
  };

  // Create a new EMPTY step
  static std::shared_ptr<Step> create() {
    auto ret = std::shared_ptr<Step>{new Step()};
    ret->self = ret;
    return ret;
  }

  // Create a new step
  static std::shared_ptr<Step> create(std::span<const std::string> args) {
    auto ret = std::shared_ptr<Step>{new Step(args)};
    ret->self = ret;
    return ret;
  }

  // Create a new step
  static std::shared_ptr<Step> create(std::initializer_list<std::string> args) {
    return create(std::span<const std::string>{args.begin(), args.size()});
  }

  // Create a new EMPTY task, put previous as the dependency and return this new
  // task.
  std::shared_ptr<Step> add() {
    auto s = create();
    s->depend_on(self.lock());
    return s;
  }

  // Create a new task, put previous as the dependency and return this new task.
  std::shared_ptr<Step> add(std::span<const std::string> args) {
    auto s = create(std::move(args));
    s->depend_on(self.lock());
    return s;
  }

  // Create a new task, put previous as the dependency and return this new task.
  std::shared_ptr<Step> add(std::initializer_list<std::string> args) {
    return add(std::span<const std::string>{args.begin(), args.size()});
  }

  // Add a dependency. Dependencies run before this step
  std::shared_ptr<Step> depend_on(std::shared_ptr<Step> other) {
    dependencies.push_back(other);
    return self.lock();
  }

  // Wait for dependencies and then run this step on a new thread.
  //
  // Maybe this function should be rewritten to not block on waiting for
  // dependencies?
  void run() {
    if (status != READY)
      return;

    {
      std::vector<std::future<void>> futures;

      for (auto &dep : dependencies) {
        futures.push_back(std::async([&dep]() {
          dep->run();
          dep->get_future().wait();
        }));
      }
      for (auto &f : futures) {
        f.wait();
      }
    }

    status = RUNNING;
    if (args.size() > 0) {
      std::cout << "nopp: ";
      for (auto &a : args) {
        if (has_space(a))
          std::cout << '\'' << a << "' ";
        else
          std::cout << a << ' ';
      }
      std::cout << '\n';

      running_thread = std::jthread{[this]() {
        pid_t pid = fork();
        if (pid != 0) {
          waitpid(pid, nullptr, 0);
        } else {
          std::vector<char *> buf;
          for (auto &a : args) {
            buf.push_back(const_cast<char *>(a.c_str()));
          }
          buf.push_back(0);
          execvp(args[0].c_str(), buf.data());
        }

        promise.set_value();
        status = DONE;
      }};
    } else {
      promise.set_value();
      status = DONE;
    }
  }

  // Get a future of this step.
  std::shared_future<void> get_future() { return future; }

private:
  // Step that does nothing
  Step() { future = promise.get_future().share(); }

  // Run the process args[0] with args[1..] arguments
  explicit Step(std::span<const std::string> args) {
    this->args.reserve(args.size());
    for (auto &a : args) {
      this->args.push_back(a);
    }
    future = promise.get_future().share();
  }

  std::weak_ptr<Step> self;

  Status status = READY;
  std::vector<std::shared_ptr<Step>> dependencies;
  std::vector<std::string> args;
  std::shared_future<void> future;
  std::promise<void> promise;
  std::jthread running_thread;
};

// Main class
// Stores the building state
class NOPP {
public:
  NOPP(int argc, char *argv[]) {
    this->argv = argv;
    default_action = Step::create();
    actions.insert({"default", default_action});

    rebuildng_urself_step =
        Step::create({"g++", "-std=c++26", "-o", argv[0], "nopp.cpp"});
    action("rebuild_nopp")->depend_on(rebuildng_urself_step);

    argc -= 1; // prog name
    argv += 1;

    if (argc > 0) {
      if (argv[0][0] != '-') {
        selected_action = argv[0];

        argc -= 1;
        argv += 1;
      }
    }

    std::pair<std::string, std::string> new_option;
    bool parsed_name = false;

    while (argc > 0) {
      std::string arg = argv[0];

      if (parsed_name == false) {
        if (arg[0] != '-') {
          std::cerr << "Option didn't start with '-'\n";
          exit(1);
        }

        if (arg.size() == 1) {
          std::cerr << "Option is just '-'\n";
          exit(1);
        }

        if (arg[1] != '-') {
          for (int i = 1; i < arg.size(); ++i) {
            options.push_back({std::string{"-"} + arg[i], ""});
          }
          argc -= 1;
          argv += 1;
          continue;
        }

        new_option.first = arg;
        parsed_name = true;
        argc -= 1;
        argv += 1;
        continue;
      }

      if (arg[0] != '-') {
        new_option.second = arg;
        argc -= 1;
        argv += 1;
      }

      options.push_back(new_option);
      new_option = {};
      parsed_name = false;
    }

    if (parsed_name) {
      options.push_back(new_option);
      new_option = {};
    }
  }

  // Read a value of some option provided to the `./nopp`
  template <class T> std::optional<T> option(std::string_view str);

  // Rebuild `./nopp` if nopp.cpp changed
  void rebuild_urself() {
    auto nopp_time = std::filesystem::last_write_time(argv[0]);
    auto nopp_source_time = std::filesystem::last_write_time("nopp.cpp");

    if (nopp_time > nopp_source_time)
      return;

    rebuildng_urself_step->run();
    rebuildng_urself_step->get_future().wait();
    execvp(argv[0], argv);
  }

  // It's buildin' time!
  void build() { actions.at(selected_action)->run(); }

  // Get default action
  // Add steps as dependencies to run them when action runs
  std::shared_ptr<Step> get_default() { return default_action; }
  // Create a new action runnable using `./nopp action_name`
  std::shared_ptr<Step> action(std::string_view str) {
    auto a = Step::create();
    actions.insert({std::string{str}, a});
    return a;
  }

private:
  std::string selected_action = "default";
  std::map<std::string, std::shared_ptr<Step>> actions;

  std::shared_ptr<Step> default_action;
  std::shared_ptr<Step> rebuildng_urself_step;

  std::vector<std::pair<std::string, std::string>> options;
  char **argv;
};

} // namespace NOPP

#ifdef NOPP_IMPLEMENTATION
#include "impl.cpp"
#endif
