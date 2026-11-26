#pragma once
#include <filesystem>
#include <future>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
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

  template <class... Args> static std::shared_ptr<Step> create(Args &&...args) {
    auto ret = std::shared_ptr<Step>{new Step(std::forward<Args>(args)...)};
    ret->self = ret;
    return ret;
  }

  static std::shared_ptr<Step>
  create(std::initializer_list<std::string> &&args) {
    auto ret = std::shared_ptr<Step>{new Step(std::move(args))};
    ret->self = ret;
    return ret;
  }

  template <class... Args> std::shared_ptr<Step> add(Args &&...args) {
    auto s = create(std::forward<Args>(args)...);
    s->depend_on(self.lock());
    return s;
  }

  std::shared_ptr<Step> add(std::initializer_list<std::string> &&args) {
    auto s = create(std::move(args));
    s->depend_on(self.lock());
    return s;
  }

  std::shared_ptr<Step> depend_on(std::shared_ptr<Step> other) {
    dependencies.push_back(other);
    return self.lock();
  }

  void run() {
    if (status != READY)
      return;

    for (auto &dep : dependencies) {
      dep->run();
    }
    for (auto &dep : dependencies) {
      dep->get_future().wait();
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

  std::shared_future<void> get_future() { return future; }

private:
  // Step that does nothing
  Step() { future = promise.get_future().share(); }

  // Run the process args[0] with args[1..] arguments
  explicit Step(std::initializer_list<std::string> args) : args{args} {
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

    if (argc > 1) {
      // TODO: make it better
      selected_action = argv[1];
    }
  }

  void rebuild_urself() {
    auto nopp_time = std::filesystem::last_write_time(argv[0]);
    auto nopp_source_time = std::filesystem::last_write_time("nopp.cpp");

    if (nopp_time > nopp_source_time)
      return;

    rebuildng_urself_step->run();
    rebuildng_urself_step->get_future().wait();
    execvp(argv[0], argv);
  }

  void build() { actions.at(selected_action)->run(); }

  std::shared_ptr<Step> get_default() { return default_action; }
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

  char **argv;
};

} // namespace NOPP

#ifdef NOPP_IMPLEMENTATION
#include "impl.cpp"
#endif
