#include "app.hpp"
#include "yasr.hpp"
#include "yasr_raii.hpp"

#include <beyond/math/function.hpp>
#include <beyond/utils/bit_cast.hpp>
#include <beyond/utils/panic.hpp>

#include <SDL2/SDL_image.h>
#include <spdlog/spdlog.h>
#include <tiny_obj_loader.h>

namespace {

using beyond::float_constants::pi;

constexpr auto rgb_to_uint32(const RGB& c) noexcept -> uint32_t
{
  const auto r = static_cast<uint8_t>(c.r * 255.99f);
  const auto g = static_cast<uint8_t>(c.g * 255.99f);
  const auto b = static_cast<uint8_t>(c.b * 255.99f);
  return r << 16u | g << 8u | b;
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

} // namespace

App::App()
    : image_{width, height},
      depth_buffer_(width * height, -std::numeric_limits<float>::infinity())
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

  constexpr const char* model_filename = "assets/model/african_head.obj";

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, model_filename)) {
    beyond::panic(err);
  }

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      vertices.emplace_back(Vertex{
          .pos = {attrib.vertices[3 * index.vertex_index + 0],
                  attrib.vertices[3 * index.vertex_index + 1],
                  attrib.vertices[3 * index.vertex_index + 2]},
          .normal =
              {
                  attrib.normals[3 * index.normal_index + 0],
                  attrib.normals[3 * index.normal_index + 1],
                  attrib.normals[3 * index.normal_index + 2],
              },
          .texcoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                       attrib.texcoords[2 * index.texcoord_index + 1]},
      });
      indices.push_back(indices.size());
    }
  }

  auto device = yasr::Device::create();

  auto vertex_buffer = yasr::create_unique_buffer(
      *device,
      yasr::BufferDesc{
          .data = std::span(beyond::bit_cast<std::byte*>(vertices.data()),
                            vertices.size() * sizeof(Vertex))});
  auto index_buffer = yasr::create_unique_buffer(
      *device,
      yasr::BufferDesc{
          .data = std::span(beyond::bit_cast<std::byte*>(indices.data()),
                            indices.size() * sizeof(uint32_t))});

  device->bind_vertex_buffer(vertex_buffer);
  device->bind_index_buffer(index_buffer);
  device->draw_indexed(image_, depth_buffer_);
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