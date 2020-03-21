#include "app.hpp"

#include <spdlog/spdlog.h>

#include <beyond/core/math/vector.hpp>
#include <beyond/core/utils/bit_cast.hpp>

#include <SDL2/SDL_image.h>

constexpr int width = 1200;
constexpr int height = 800;

namespace {

constexpr auto rgb_to_uint32(const RGB& c) noexcept -> uint32_t
{
  const auto r = static_cast<uint8_t>(c.r * 255.99f);
  const auto g = static_cast<uint8_t>(c.g * 255.99f);
  const auto b = static_cast<uint8_t>(c.b * 255.99f);
  return r << 16 | g << 8 | b;
}

auto copy_to_screen(const Image& image, SDL_Texture* window_texture) noexcept
    -> void
{
  void* pixels;
  int pitch;
  if (SDL_LockTexture(window_texture, nullptr, &pixels, &pitch) != 0) {
    spdlog::error("[SDL2] Couldn't lock the screen texture: {}",
                  SDL_GetError());
  }

  for (int y = 0; y < height; ++y) {
    uint32_t* dst = beyond::bit_cast<uint32_t*>(
        (static_cast<uint8_t*>(pixels) + y * pitch));
    for (int x = 0; x < width; ++x) {
      *dst++ = rgb_to_uint32(image.unsafe_at(x, y));
    }
  }

  SDL_UnlockTexture(window_texture);
}

} // namespace

App::App() : image_{width, height}
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    spdlog::critical("[SDL2] Unable to initialize SDL: {}", SDL_GetError());
    std::exit(1);
  }

  if (SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_RESIZABLE, &window_,
                                  &renderer_)) {
    spdlog::critical("[SDL2] Couldn't create window and renderer: {}",
                     SDL_GetError());
    std::exit(1);
  }
  SDL_SetWindowResizable(window_, SDL_FALSE);
  SDL_SetWindowTitle(window_, "Yet Another Software Rasterizer");

  window_texture_ =
      SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGB888,
                        SDL_TEXTUREACCESS_STREAMING, width, height);
  if (!window_texture_) {
    spdlog::critical("[SDL2] Couldn't create texture: {}", SDL_GetError());
    std::exit(1);
  }
}

App::~App()
{
  SDL_DestroyTexture(window_texture_);

  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

auto App::update(const Milliseconds& delta_time) -> void
{
  handle_input();
  render(delta_time);
}

auto line(beyond::IPoint2 p0, beyond::IPoint2 p1, Image& image,
          const RGB& color) -> void
{
  bool steep = false;
  if (std::abs(p0.x - p1.x) <
      std::abs(p0.y - p1.y)) { // if the line is steep, we transpose the image
    p0.xy = p0.yx;
    p1.xy = p1.yx;
    steep = true;
  }
  if (p0.x > p1.x) { // make it left−to−right
    std::swap(p0, p1);
  }
  for (int x = p0.x; x <= p1.x; x++) {
    float t = (x - p0.x) / static_cast<float>(p1.x - p0.x);
    int y = p0.y * (1. - t) + p1.y * t;
    if (steep) {
      image.unsafe_at(y, x) = color; // if transposed, de−transpose
    } else {
      image.unsafe_at(x, y) = color;
    }
  }
}

auto App::render(const Milliseconds & /*delta_time*/) -> void
{
  constexpr auto white = RGB(1, 1, 1);
  constexpr auto red = RGB(1, 0, 0);

  line({13, 20}, {80, 40}, image_, white);
  line({20, 13}, {40, 80}, image_, red);
  line({80, 40}, {13, 20}, image_, red);

  copy_to_screen(image_, window_texture_);

  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, window_texture_, nullptr, nullptr);
  SDL_RenderPresent(renderer_);
}

auto App::handle_input() -> void
{
  SDL_Event sdl_event;
  while (SDL_PollEvent(&sdl_event) != 0) {
    switch (sdl_event.type) {
    case SDL_KEYDOWN:
      if (sdl_event.key.keysym.sym == SDLK_ESCAPE) {
        should_close_ = true;
      }
      break;
    case SDL_QUIT:
      should_close_ = true;
      break;
    }
  }
}