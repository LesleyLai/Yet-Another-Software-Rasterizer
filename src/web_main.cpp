#include "common/app.hpp"

#include <functional>

#include <emscripten.h>

std::function<void()> loop;
auto main_loop() -> void
{
  loop();
}

int main()
{
  App app;

  auto now = std::chrono::high_resolution_clock::now();
  auto previous = now;

  loop = [&] {
    now = std::chrono::high_resolution_clock::now();
    app.update(now - previous);
    previous = now;
  };

  emscripten_set_main_loop(main_loop, 0, true);

  return EXIT_SUCCESS;
}