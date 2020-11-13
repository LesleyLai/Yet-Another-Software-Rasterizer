#pragma once

#ifndef YASR_APP_HPP
#define YASR_APP_HPP

#include <SDL2/SDL.h>

#include <array>
#include <chrono>
#include <vector>

#include "image.hpp"

#include "yasr.hpp"

using Milliseconds = std::chrono::duration<double, std::milli>;

class App {
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  SDL_Texture* window_texture_ = nullptr;
  bool should_close_ = false;

  Image image_;
  std::vector<float> depth_buffer_;

public:
  App();
  ~App();

  App(const App&) = delete;
  auto operator=(const App&) & noexcept = delete;
  App(App&&) noexcept = delete;
  auto operator=(App&&) & noexcept = delete;

  auto update(const Milliseconds& delta_time) -> void;
  auto render(const Milliseconds& delta_time) -> void;
  auto handle_input() -> void;

  [[nodiscard]] auto should_close() const -> bool
  {
    return should_close_;
  }
};

#endif // YASR_APP_HPP
