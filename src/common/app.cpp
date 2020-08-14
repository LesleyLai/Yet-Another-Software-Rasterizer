#include "app.hpp"

#include <spdlog/spdlog.h>

#include <beyond/core/math/function.hpp>
#include <beyond/core/math/serial.hpp>
#include <beyond/core/math/vector.hpp>
#include <beyond/core/utils/bit_cast.hpp>

#include <fmt/format.h>

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
  void* pixels = nullptr;
  int pitch = 0;
  if (SDL_LockTexture(window_texture, nullptr, &pixels, &pitch) != 0) {
    spdlog::error("[SDL2] Couldn't lock the screen texture: {}",
                  SDL_GetError());
  }

  for (int y = 0; y < height; ++y) {
    auto* dst = beyond::bit_cast<uint32_t*>(
        (static_cast<uint8_t*>(pixels) + y * pitch));
    for (int x = 0; x < width; ++x) {
      *dst++ = rgb_to_uint32(image.unsafe_at(x, y));
    }
  }

  SDL_UnlockTexture(window_texture);
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
    const auto t =
        static_cast<float>(x - p0.x) / static_cast<float>(p1.x - p0.x);
    const auto y = static_cast<int>(std::round(p0.y * (1.f - t) + p1.y * t));
    if (steep) {
      if (y >= 0 && y < image.width() && x >= 0 && x < image.height()) {
        image.unsafe_at(y, x) = color; // if transposed, de−transpose
      }
    } else {
      if (x >= 0 && x < image.width() && y >= 0 && y < image.height()) {
        image.unsafe_at(x, y) = color;
      }
    }
  }
}

constexpr auto point3_to_screen(const beyond::Point3& pt)
{
  return beyond::IPoint2{static_cast<int>((pt.x + 1.f) * width / 2),
                         height - static_cast<int>((pt.y + 1.f) * height / 2)};
}

} // namespace

App::App() : image_{width, height}
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    spdlog::critical("[SDL2] Unable to initialize SDL: {}", SDL_GetError());
    std::exit(1);
  }

  if (SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_RESIZABLE, &window_,
                                  &renderer_) != 0) {
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

  /*
  const char* filename = "assets/model/african_head.obj";

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename)) {
    throw std::runtime_error(err);
  }

  constexpr auto white = RGB(1, 1, 1);
  for (const auto& shape : shapes) {
    for (std::size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
      const auto index = shape.mesh.indices[i];
      const auto index2 = shape.mesh.indices[i + 1];
      const auto index3 = shape.mesh.indices[i + 2];

      const beyond::Point3 pt1{attrib.vertices[3 * index.vertex_index],
                               attrib.vertices[3 * index.vertex_index + 1],
                               attrib.vertices[3 * index.vertex_index + 2]};

      const auto pt2 = *beyond::bit_cast<beyond::Point3*>(
          &attrib.vertices[3 * index2.vertex_index]);
      const auto pt3 = *beyond::bit_cast<beyond::Point3*>(
          &attrib.vertices[3 * index3.vertex_index]);

      const beyond::IPoint2 pt1_screen = point3_to_screen(pt1);
      const beyond::IPoint2 pt2_screen = point3_to_screen(pt2);
      const beyond::IPoint2 pt3_screen = point3_to_screen(pt3);
      line(pt1_screen, pt2_screen, image_, white);
      line(pt2_screen, pt3_screen, image_, white);
      line(pt3_screen, pt1_screen, image_, white);
    }
  }
   */
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

void triangle(beyond::IVec2 t0, beyond::IVec2 t1, beyond::IVec2 t2,
              Image& image, RGB border_color, RGB fill_color = RGB{1, 1, 1})
{
  if (t0.y > t1.y) {
    std::swap(t0, t1);
  }
  if (t0.y > t2.y) {
    std::swap(t0, t2);
  }
  if (t1.y > t2.y) {
    std::swap(t1, t2);
  }

  const int total_height = t2.y - t0.y;
  for (int y = t0.y; y < t2.y; ++y) {
    const bool first_half = y < t1.y;
    const int segment_height = (first_half ? t1.y - t0.y : t2.y - t1.y) + 1;
    const auto alpha =
        static_cast<float>(y - t0.y) / static_cast<float>(total_height);
    const auto beta = (first_half ? static_cast<float>(y - t0.y)
                                  : static_cast<float>(t2.y - y)) /
                      static_cast<float>(segment_height);

    auto x0 = static_cast<int>(beyond::lerp(static_cast<float>(t2.x),
                                            static_cast<float>(t0.x), alpha));
    auto x1 =
        first_half
            ? static_cast<int>(beyond::lerp(static_cast<float>(t1.x),
                                            static_cast<float>(t0.x), beta))
            : static_cast<int>(beyond::lerp(static_cast<float>(t1.x),
                                            static_cast<float>(t2.x), beta));
    if (x1 < x0) {
      std::swap(x0, x1);
    }

    for (int x = x0; x <= x1; ++x) {
      image.unsafe_at(x, y) = fill_color;
    }
  }

  for (int y = t0.y; y < t2.y; ++y) {
    const bool first_half = y < t1.y;
    const int segment_height = (first_half ? t1.y - t0.y : t2.y - t1.y) + 1;
    const auto alpha =
        static_cast<float>(y - t0.y) / static_cast<float>(total_height);
    const auto beta = (first_half ? static_cast<float>(y - t0.y)
                                  : static_cast<float>(t2.y - y)) /
                      static_cast<float>(segment_height);

    auto x0 = static_cast<int>(beyond::lerp(static_cast<float>(t2.x),
                                            static_cast<float>(t0.x), alpha));
    auto x1 =
        first_half
            ? static_cast<int>(beyond::lerp(static_cast<float>(t1.x),
                                            static_cast<float>(t0.x), beta))
            : static_cast<int>(beyond::lerp(static_cast<float>(t1.x),
                                            static_cast<float>(t2.x), beta));
    if (x1 < x0) {
      std::swap(x0, x1);
    }

    for (int x = x0; x <= x1; ++x) {
      image.unsafe_at(x, y) = fill_color;
    }
  }

  line(t0, t1, image, border_color);
  line(t1, t2, image, border_color);
  line(t2, t0, image, border_color);
}

auto App::render(const Milliseconds& /*delta_time*/) -> void
{

  constexpr beyond::IVec2 t0[3] = {{10, 70}, {50, 160}, {70, 80}};
  constexpr beyond::IVec2 t1[3] = {{180, 50}, {150, 1}, {70, 180}};
  constexpr beyond::IVec2 t2[3] = {{180, 150}, {120, 160}, {130, 180}};
  triangle(t0[0], t0[1], t0[2], image_, RGB{1, 0, 0});
  triangle(t1[0], t1[1], t1[2], image_, RGB{0, 0, 1});
  triangle(t2[0], t2[1], t2[2], image_, RGB{0, 1, 0});

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