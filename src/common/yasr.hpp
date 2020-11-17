#ifndef YASR_HPP
#define YASR_HPP

#include "image.hpp"

#include <cstdint>
#include <memory>
#include <span>

#include <beyond/math/point.hpp>

constexpr int width = 1200;
constexpr int height = 800;

struct Vertex {
  beyond::Point3 pos;
  beyond::Vec3 normal;
  beyond::Vec2 texcoord;
};

namespace yasr {

#define DEFINE_HANDLE(type)                                                    \
  struct type {                                                                \
    std::uint64_t id = 0;                                                      \
                                                                               \
    [[nodiscard]] constexpr friend auto operator==(type lhs, type rhs)         \
        -> bool = default;                                                     \
  };

struct Device;

DEFINE_HANDLE(Buffer)

struct BufferDesc {
  std::span<const std::byte> data;
};

struct Device {
  [[nodiscard]] static auto create() -> std::unique_ptr<Device>;

  [[nodiscard]] virtual auto create_buffer(BufferDesc desc) -> Buffer = 0;
  virtual void destroy_buffer(Buffer buffer) = 0;

  virtual void bind_vertex_buffer(Buffer vertex_buffer) = 0;
  virtual void bind_index_buffer(Buffer index_buffer) = 0;
  virtual void draw_indexed(Image& image, std::vector<float> depth_buffer) = 0;

  Device() = default;
  virtual ~Device() = default;
  Device(const Device&) = delete;
  auto operator=(const Device&) & -> Device& = delete;
  Device(Device&&) noexcept = default;
  auto operator=(Device&&) & noexcept -> Device& = default;
};

} // namespace yasr

#endif // YASR_HPP
