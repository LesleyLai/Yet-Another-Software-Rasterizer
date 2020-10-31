#pragma once

#ifndef YASR_APP_HPP
#define YASR_APP_HPP

#include <SDL2/SDL.h>

#include <array>
#include <chrono>
#include <vector>

#include "image.hpp"

#include <tiny_obj_loader.h>

using Milliseconds = std::chrono::duration<double, std::milli>;

class App {
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  SDL_Texture* window_texture_ = nullptr;
  bool should_close_ = false;

  float* diffuse_texture_ = nullptr;
  int diffuse_texture_width_ = 0;
  int diffuse_texture_height_ = 0;
  int diffuse_texture_channels_ = 0;

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
