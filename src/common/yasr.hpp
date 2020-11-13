#ifndef YASR
#define YASR

#include "image.hpp"

#include <cstdint>
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

[[nodiscard]] auto create_device() -> Device*;
void destroy_device(Device* device);

struct BufferDesc {
  std::span<const std::byte> data;
};
[[nodiscard]] auto create_buffer(Device& device, BufferDesc desc) -> Buffer;
void destroy_buffer(Device& device, Buffer buffer);

void bind_vertex_buffer(Device& device, Buffer vertex_buffer);
void bind_index_buffer(Device& device, Buffer index_buffer);
void draw_indexed(Device& device, Image& image,
                  std::vector<float> depth_buffer);

} // namespace yasr

#endif // YASR
