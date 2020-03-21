#include "common/app.hpp"

int main()
{
  App app;

  auto now = std::chrono::high_resolution_clock::now();
  auto previous = now;

  while (!app.should_close()) {
    now = std::chrono::high_resolution_clock::now();
    app.update(now - previous);
    previous = now;
  }

  return 0;
}