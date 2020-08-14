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

[[maybe_unused]] auto line(beyond::IPoint2 p0, beyond::IPoint2 p1, Image& image,
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

beyond::Vec3 barycentric(const std::array<beyond::IVec2, 3>& pts,
                         beyond::IVec2 p)
{
  const auto u = cross(beyond::Vec3(pts[2][0] - pts[0][0],
                                    pts[1][0] - pts[0][0], pts[0][0] - p[0]),
                       beyond::Vec3(pts[2][1] - pts[0][1],
                                    pts[1][1] - pts[0][1], pts[0][1] - p[1]));
  // Returns negative values for degenerated triangle
  if (u[2] == 0) {
    return beyond::Vec3(-1, 1, 1);
  }
  return beyond::Vec3(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}

void triangle(const std::array<beyond::IVec2, 3>& pts, Image& image, RGB color)
{
  const beyond::IVec2 bbox_upper_bound{image.width() - 1, image.height() - 1};
  beyond::IVec2 bboxmin{bbox_upper_bound};
  beyond::IVec2 bboxmax{0, 0};
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 2; ++j) {
      bboxmin[j] = std::clamp(bboxmin[j], 0, pts[i][j]);
      bboxmax[j] = std::clamp(bboxmax[j], pts[i][j], bbox_upper_bound[j]);
    }
  }

  for (int x = bboxmin.x; x < bboxmax.x; ++x) {
    for (int y = bboxmin.y; y < bboxmax.y; ++y) {
      const beyond::Vec3 bc_screen = barycentric(pts, {x, y});
      if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
        continue;
      image.unsafe_at(x, y) = color;
    }
  }
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

  const char* filename = "assets/model/african_head.obj";

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename)) {
    throw std::runtime_error(err);
  }

  const auto light_dir = beyond::normalize(beyond::Vec3{0, 1, 5});
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

      const auto normal =
          beyond::normalize(beyond::cross(pt2 - pt1, pt3 - pt2));
      float intensity = beyond::dot(normal, light_dir);
      if (intensity > 0) {
        triangle({pt1_screen, pt2_screen, pt3_screen}, image_,
                 RGB(intensity, intensity, intensity));
      }
    }
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

auto App::render(const Milliseconds& /*delta_time*/) -> void
{
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